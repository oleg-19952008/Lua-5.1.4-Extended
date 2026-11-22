/*
** lxmllib.c — XML module for Lua 5.1 (ooxi/xml.c)
** parse + path (simple xpath-like) + safe variants
*/
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "xml.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* ---------- node → table ---------- */

static void push_xml_string(lua_State *L, struct xml_string *s) {
  size_t len;
  char *buf;
  if (!s) { lua_pushnil(L); return; }
  len = xml_string_length(s);
  buf = (char *)malloc(len + 1);
  if (!buf) luaL_error(L, "xml: out of memory");
  xml_string_copy(s, (uint8_t *)buf, len);
  buf[len] = '\0';
  lua_pushlstring(L, buf, len);
  free(buf);
}

static void push_node(lua_State *L, struct xml_node *node) {
  size_t i, n;
  lua_newtable(L);

  push_xml_string(L, xml_node_name(node));
  lua_setfield(L, -2, "name");

  {
    struct xml_string *c = xml_node_content(node);
    if (c) {
      push_xml_string(L, c);
      lua_setfield(L, -2, "content");
    }
  }

  n = xml_node_attributes(node);
  lua_newtable(L);
  for (i = 0; i < n; ++i) {
    lua_newtable(L);
    push_xml_string(L, xml_node_attribute_name(node, i));
    lua_setfield(L, -2, "name");
    push_xml_string(L, xml_node_attribute_content(node, i));
    lua_setfield(L, -2, "content");
    lua_rawseti(L, -2, (int)(i + 1));
  }
  lua_setfield(L, -2, "attr");

  /* dict form: attrs["id"] */
  lua_newtable(L);
  for (i = 0; i < n; ++i) {
    push_xml_string(L, xml_node_attribute_name(node, i));
    push_xml_string(L, xml_node_attribute_content(node, i));
    lua_rawset(L, -3);
  }
  lua_setfield(L, -2, "attrs");

  n = xml_node_children(node);
  lua_newtable(L);
  for (i = 0; i < n; ++i) {
    push_node(L, xml_node_child(node, i));
    lua_rawseti(L, -2, (int)(i + 1));
  }
  lua_setfield(L, -2, "children");
}

/* ---------- parse ---------- */

static int l_parse(lua_State *L) {
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  uint8_t *buf;
  struct xml_document *doc;
  struct xml_node *root;

  buf = (uint8_t *)malloc(len + 1);
  if (!buf) return luaL_error(L, "xml: out of memory");
  memcpy(buf, s, len);
  buf[len] = '\0';

  doc = xml_parse_document(buf, len);
  if (!doc) {
    free(buf);
    return luaL_error(L, "xml: parse error");
  }
  root = xml_document_root(doc);
  if (!root) {
    xml_document_free(doc, 1);
    return luaL_error(L, "xml: empty document");
  }
  push_node(L, root);
  xml_document_free(doc, 1);
  return 1;
}

/* ---------- path helpers ---------- */

static int match_attr(lua_State *L, int node_idx, const char *aname, const char *aval) {
  int ok = 0;
  lua_getfield(L, node_idx, "attrs");
  if (!lua_istable(L, -1)) { lua_pop(L, 1); return 0; }
  lua_getfield(L, -1, aname);
  if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), aval) == 0)
    ok = 1;
  lua_pop(L, 2);
  return ok;
}

static void collect_by_name(lua_State *L, int node_idx, const char *name,
                            int has_pred, const char *aname, const char *aval,
                            int out_idx) {
  size_t i, n;
  int children_idx, child_idx;
  lua_getfield(L, node_idx, "children");
  if (!lua_istable(L, -1)) { lua_pop(L, 1); return; }
  children_idx = lua_gettop(L);
  n = lua_objlen(L, children_idx);
  for (i = 1; i <= n; ++i) {
    lua_rawgeti(L, children_idx, (int)i);
    child_idx = lua_gettop(L);
    lua_getfield(L, child_idx, "name");
    if (lua_isstring(L, -1) && strcmp(lua_tostring(L, -1), name) == 0) {
      if (!has_pred || match_attr(L, child_idx, aname, aval)) {
        lua_pushvalue(L, child_idx);
        lua_rawseti(L, out_idx, (int)(lua_objlen(L, out_idx) + 1));
      }
    }
    lua_pop(L, 2); /* name, child */
  }
  lua_pop(L, 1); /* children */
}

/*
  xml.path(node, "item[@id=2]")
  Segments separated by '/'. Predicate: [@attr=value] (quotes optional).
*/
static int l_path(lua_State *L) {
  const char *path;
  char *work, *p;
  int result, current;
  size_t i, cn;

  luaL_checktype(L, 1, LUA_TTABLE);
  path = luaL_checkstring(L, 2);

  /* result table */
  lua_newtable(L);
  result = lua_gettop(L);

  /* current node set = { root } */
  lua_newtable(L);
  lua_pushvalue(L, 1);
  lua_rawseti(L, -2, 1);
  current = lua_gettop(L);

  work = (char *)malloc(strlen(path) + 1);
  if (!work) return luaL_error(L, "xml: out of memory");
  strcpy(work, path);

  p = work;
  while (*p) {
    char name[256];
    char aname[128];
    char aval[256];
    int has_pred = 0;
    char *slash, *bracket, *eq, *end;
    size_t nlen, alen, vlen;
    int nextset;

    while (*p == '/') ++p;
    if (!*p) break;
    slash = strchr(p, '/');
    if (slash) *slash = '\0';

    name[0] = aname[0] = aval[0] = '\0';
    bracket = strchr(p, '[');
    if (bracket) {
      nlen = (size_t)(bracket - p);
      end = strchr(bracket, ']');
      eq = strchr(bracket, '=');
      if (nlen == 0 || nlen >= sizeof(name) || !end || end[1] != '\0' ||
          bracket[1] != '@' || !eq || eq >= end || eq == bracket + 2)
        { free(work); return luaL_error(L, "xml.path: invalid predicate"); }
      alen = (size_t)(eq - (bracket + 2));
      vlen = (size_t)(end - (eq + 1));
      if (alen >= sizeof(aname) || vlen >= sizeof(aval) || vlen == 0)
        { free(work); return luaL_error(L, "xml.path: predicate too long or empty"); }
      memcpy(name, p, nlen); name[nlen] = '\0';
      memcpy(aname, bracket + 2, alen); aname[alen] = '\0';
      memcpy(aval, eq + 1, vlen); aval[vlen] = '\0';
      if (aval[0] == '\'' || aval[0] == '"') {
        size_t qlen = strlen(aval);
        if (qlen < 2 || aval[qlen - 1] != aval[0])
          { free(work); return luaL_error(L, "xml.path: mismatched predicate quotes"); }
        memmove(aval, aval + 1, qlen - 2);
        aval[qlen - 2] = '\0';
      } else if (strchr(aval, '\'') || strchr(aval, '"')) {
        free(work); return luaL_error(L, "xml.path: malformed predicate value");
      }
      has_pred = 1;
    } else {
      nlen = strlen(p);
      if (nlen == 0 || nlen >= sizeof(name))
        { free(work); return luaL_error(L, "xml.path: element name too long or empty"); }
      memcpy(name, p, nlen + 1);
    }

    /* build next set */
    lua_newtable(L);
    nextset = lua_gettop(L);
    cn = lua_objlen(L, current);
    for (i = 1; i <= cn; ++i) {
      lua_rawgeti(L, current, (int)i);
      collect_by_name(L, lua_gettop(L), name, has_pred, aname, aval, nextset);
      lua_pop(L, 1);
    }
    /* drop old current, keep nextset as new current */
    lua_remove(L, current);
    current = nextset - 1; /* after remove, indices above current shift down by 1 */

    if (slash) p = slash + 1;
    else break;
  }
  free(work);

  /* copy current → result */
  cn = lua_objlen(L, current);
  for (i = 1; i <= cn; ++i) {
    lua_rawgeti(L, current, (int)i);
    lua_rawseti(L, result, (int)i);
  }
  lua_settop(L, result);
  return 1;
}

/* ---------- safe ---------- */

static int safe_call(lua_State *L, lua_CFunction f, int nresults) {
  int n = lua_gettop(L);
  lua_pushcfunction(L, f);
  lua_insert(L, 1);
  if (lua_pcall(L, n, nresults, 0) != 0) {
    lua_pushnil(L);
    lua_insert(L, -2);
    return 2;
  }
  return nresults;
}

static int l_parse_safe(lua_State *L) { return safe_call(L, l_parse, 1); }
static int l_path_safe (lua_State *L) { return safe_call(L, l_path,  1); }

static const luaL_Reg xmllib[] = {
  {"parse",      l_parse},
  {"path",       l_path},
  {"parse_safe", l_parse_safe},
  {"path_safe",  l_path_safe},
  {NULL, NULL}
};

LUALIB_API int luaopen_xml(lua_State *L) {
  luaL_register(L, LUA_XMLLIBNAME, xmllib);
  return 1;
}
