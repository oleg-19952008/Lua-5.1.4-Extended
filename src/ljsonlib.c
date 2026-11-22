/*
** ljsonlib.c — JSON module for Lua 5.1 (cJSON)
** encode / decode / encode_pretty + safe variants
*/
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* ---------- helpers: cJSON -> Lua ---------- */

static void push_cjson(lua_State *L, cJSON *item);

static void push_object(lua_State *L, cJSON *obj) {
  cJSON *child;
  lua_newtable(L);
  for (child = obj->child; child; child = child->next) {
    lua_pushstring(L, child->string ? child->string : "");
    push_cjson(L, child);
    lua_rawset(L, -3);
  }
}

static void push_array(lua_State *L, cJSON *arr) {
  cJSON *child;
  int i = 1;
  lua_newtable(L);
  for (child = arr->child; child; child = child->next, ++i) {
    push_cjson(L, child);
    lua_rawseti(L, -2, i);
  }
}

static void push_cjson(lua_State *L, cJSON *item) {
  if (!item) { lua_pushnil(L); return; }
  switch (item->type & 0xFF) {
    case cJSON_NULL:   lua_pushlightuserdata(L, (void *)luaopen_json); break;
    case cJSON_False:  lua_pushboolean(L, 0); break;
    case cJSON_True:   lua_pushboolean(L, 1); break;
    case cJSON_Number: lua_pushnumber(L, item->valuedouble); break;
    case cJSON_String: lua_pushstring(L, item->valuestring ? item->valuestring : ""); break;
    case cJSON_Array:  push_array(L, item); break;
    case cJSON_Object: push_object(L, item); break;
    default:           lua_pushnil(L); break;
  }
}

/* ---------- helpers: Lua -> cJSON ---------- */

static int table_is_array(lua_State *L, int idx) {
  size_t max = 0, count = 0;
  lua_pushnil(L);
  while (lua_next(L, idx)) {
    if (lua_type(L, -2) != LUA_TNUMBER) { lua_pop(L, 2); return 0; }
    {
      lua_Number n = lua_tonumber(L, -2);
      size_t k = (size_t)n;
      if (n != (lua_Number)k || k < 1) { lua_pop(L, 2); return 0; }
      if (k > max) max = k;
      ++count;
    }
    lua_pop(L, 1);
  }
  return (count == max && count > 0);
}

static cJSON *to_cjson(lua_State *L, int idx, const void **seen, int depth) {
  int absidx;
  if (idx < 0) absidx = lua_gettop(L) + idx + 1;
  else absidx = idx;

  switch (lua_type(L, absidx)) {
    case LUA_TNIL:     return cJSON_CreateNull();
    case LUA_TBOOLEAN: return cJSON_CreateBool(lua_toboolean(L, absidx));
    case LUA_TNUMBER:  return cJSON_CreateNumber(lua_tonumber(L, absidx));
    case LUA_TSTRING: {
      size_t len, i, cap;
      const unsigned char *src = (const unsigned char *)lua_tolstring(L, absidx, &len);
      char *raw;
      size_t pos = 0;
      static const char hex[] = "0123456789abcdef";
      if (len > ((size_t)-1 - 3) / 6) return NULL;
      cap = len * 6 + 3;
      raw = (char *)malloc(cap);
      if (!raw) return NULL;
      raw[pos++] = '"';
      for (i = 0; i < len; ++i) {
        unsigned char c = src[i];
        if (c == '"' || c == '\\') { raw[pos++] = '\\'; raw[pos++] = (char)c; }
        else if (c == '\b') { raw[pos++]='\\'; raw[pos++]='b'; }
        else if (c == '\f') { raw[pos++]='\\'; raw[pos++]='f'; }
        else if (c == '\n') { raw[pos++]='\\'; raw[pos++]='n'; }
        else if (c == '\r') { raw[pos++]='\\'; raw[pos++]='r'; }
        else if (c == '\t') { raw[pos++]='\\'; raw[pos++]='t'; }
        else if (c < 0x20) { raw[pos++]='\\'; raw[pos++]='u'; raw[pos++]='0'; raw[pos++]='0'; raw[pos++]=hex[c >> 4]; raw[pos++]=hex[c & 15]; }
        else raw[pos++] = (char)c;
      }
      raw[pos++] = '"'; raw[pos] = '\0';
      { cJSON *item = cJSON_CreateRaw(raw); free(raw); return item; }
    }
    case LUA_TLIGHTUSERDATA:
      if (lua_touserdata(L, absidx) == (void *)luaopen_json) return cJSON_CreateNull();
      return cJSON_CreateNull();
    case LUA_TTABLE: {
      int j;
      const void *identity = lua_topointer(L, absidx);
      if (depth >= 128) return NULL;
      for (j = 0; j < depth; ++j)
        if (seen[j] == identity) return NULL;
      seen[depth] = identity;
      if (table_is_array(L, absidx)) {
        cJSON *arr = cJSON_CreateArray();
        size_t i, n = lua_objlen(L, absidx);
        for (i = 1; i <= n; ++i) {
          cJSON *child;
          lua_rawgeti(L, absidx, (int)i);
          child = to_cjson(L, -1, seen, depth + 1);
          lua_pop(L, 1);
          if (!child || !cJSON_AddItemToArray(arr, child)) {
            if (child) cJSON_Delete(child);
            cJSON_Delete(arr);
            return NULL;
          }
        }
        return arr;
      } else {
        cJSON *obj = cJSON_CreateObject();
        lua_pushnil(L);
        while (lua_next(L, absidx)) {
          lua_pushvalue(L, -2);
          if (lua_isstring(L, -1)) {
            const char *key = lua_tostring(L, -1);
            cJSON *child = to_cjson(L, -2, seen, depth + 1);
            if (!child || !cJSON_AddItemToObject(obj, key, child)) {
              if (child) cJSON_Delete(child);
              cJSON_Delete(obj);
              lua_pop(L, 2);
              return NULL;
            }
          }
          lua_pop(L, 2);
        }
        return obj;
      }
    }
    default:
      return cJSON_CreateNull();
  }
}

/* ---------- encode / encode_pretty ---------- */

static int encode_common(lua_State *L, int pretty) {
  const void *seen[128];
  cJSON *root = to_cjson(L, 1, seen, 0);
  char *out;
  if (!root) return luaL_error(L, "json.encode: out of memory, cyclic table, or nesting limit exceeded");
  out = pretty ? cJSON_Print(root) : cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  if (!out)
    return luaL_error(L, "json.encode: out of memory");
  lua_pushstring(L, out);
  cJSON_free(out);
  return 1;
}

static int l_encode(lua_State *L) {
  return encode_common(L, 0);
}

static int l_encode_pretty(lua_State *L) {
  return encode_common(L, 1);
}

/* ---------- decode ---------- */

static int l_decode(lua_State *L) {
  size_t len;
  const char *s = luaL_checklstring(L, 1, &len);
  cJSON *root = cJSON_ParseWithLength(s, len);
  if (!root) {
    const char *err = cJSON_GetErrorPtr();
    return luaL_error(L, "json.decode: %s", err ? err : "parse error");
  }
  push_cjson(L, root);
  cJSON_Delete(root);
  return 1;
}

/* ---------- safe wrappers ---------- */

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

static int l_encode_safe(lua_State *L) {
  return safe_call(L, l_encode, 1);
}
static int l_encode_pretty_safe(lua_State *L) {
  return safe_call(L, l_encode_pretty, 1);
}
static int l_decode_safe(lua_State *L) {
  return safe_call(L, l_decode, 1);
}

static const luaL_Reg jsonlib[] = {
  {"encode",             l_encode},
  {"encode_pretty",      l_encode_pretty},
  {"decode",             l_decode},
  {"encode_safe",        l_encode_safe},
  {"encode_pretty_safe", l_encode_pretty_safe},
  {"decode_safe",        l_decode_safe},
  {NULL, NULL}
};

LUALIB_API int luaopen_json(lua_State *L) {
  luaL_register(L, LUA_JSONLIBNAME, jsonlib);
  /* json.null sentinel */
  lua_pushlightuserdata(L, (void *)luaopen_json);
  lua_setfield(L, -2, "null");
  return 1;
}
