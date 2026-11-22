/*
** lbase64lib.c — base64 module for Lua 5.1 (NibbleAndAHalf)
*/
#define LUA_LIB

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "base64.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static int l_encode(lua_State *L) {
  size_t len;
  const char *data = luaL_checklstring(L, 1, &len);
  int outlen = 0;
  char *out;
  if (len > (((size_t)INT_MAX / 4u) * 3u))
    return luaL_error(L, "base64.encode: input too large");
  out = base64(data, (int)len, &outlen);
  if (!out)
    return luaL_error(L, "base64.encode: out of memory");
  lua_pushlstring(L, out, outlen);
  free(out);
  return 1;
}
static int l_decode_url(lua_State* L) {
    size_t len;
    size_t paddedLen;
    size_t i;
    const char* data = luaL_checklstring(L, 1, &len);
    char* normalized;
    unsigned char* out;
    int outlen = 0;

    /* Base64 length modulo 4 cannot be 1 */
    if ((len % 4) == 1)
        return luaL_error(L, "base64.decode_url: invalid input length");

    if (len > (size_t)INT_MAX - 3)
        return luaL_error(L, "base64.decode_url: input too large");

    paddedLen = len + ((4 - (len % 4)) % 4);

    normalized = (char*)malloc(paddedLen ? paddedLen : 1);
    if (!normalized)
        return luaL_error(L, "base64.decode_url: out of memory");

    /* Convert URL-safe alphabet to standard Base64 */
    for (i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)data[i];

        if (c == '-')
            normalized[i] = '+';
        else if (c == '_')
            normalized[i] = '/';
        else if ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9')) {
            normalized[i] = (char)c;
        }
        else {
            free(normalized);
            return luaL_error(L, "base64.decode_url: invalid character");
        }
    }

    /* Restore standard Base64 padding */
    while (i < paddedLen)
        normalized[i++] = '=';

    out = unbase64(normalized, (int)paddedLen, &outlen);
    free(normalized);

    if (!out)
        return luaL_error(L, "base64.decode_url: invalid input or out of memory");

    lua_pushlstring(L, (const char*)out, (size_t)outlen);
    free(out);

    return 1;
}
static int l_encode_url(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    int outlen = 0;
    char* out;
    int i;

    if (len > (((size_t)INT_MAX / 4u) * 3u))
        return luaL_error(L, "base64.encode_url: input too large");

    out = base64(data, (int)len, &outlen);
    if (!out)
        return luaL_error(L, "base64.encode_url: out of memory");

    /* RFC 4648 URL-safe alphabet */
    for (i = 0; i < outlen; ++i) {
        if (out[i] == '+')
            out[i] = '-';
        else if (out[i] == '/')
            out[i] = '_';
    }

    /* Remove trailing padding '=' */
    while (outlen > 0 && out[outlen - 1] == '=')
        --outlen;

    lua_pushlstring(L, out, (size_t)outlen);
    free(out);

    return 1;
}
static int l_decode(lua_State* L) {
    size_t len;
    const char* data = luaL_checklstring(L, 1, &len);
    int outlen = 0;
    unsigned char* out;

    if (len > (size_t)INT_MAX)
        return luaL_error(L, "base64.decode: input too large");

    /* Validation is performed inside unbase64()
       when SAFEBASE64 is enabled. */
    out = unbase64(data, (int)len, &outlen);

    if (!out)
        return luaL_error(L, "base64.decode: invalid input or out of memory");

    lua_pushlstring(L, (const char*)out, (size_t)outlen);
    free(out);

    return 1;
}

static int l_isvalid(lua_State *L) {
  size_t len;
  const char *data = luaL_checklstring(L, 1, &len);
  if (len > (size_t)INT_MAX) { lua_pushboolean(L, 0); return 1; }
  lua_pushboolean(L, base64integrity(data, (int)len));
  return 1;
}

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

static int l_encode_safe(lua_State *L) { return safe_call(L, l_encode, 1); }
static int l_decode_safe(lua_State *L) { return safe_call(L, l_decode, 1); }

static const luaL_Reg base64lib[] = {
    {"encode",      l_encode},
    {"decode",      l_decode},
    {"encode_url",  l_encode_url},
    {"decode_url",  l_decode_url},
    {"isvalid",     l_isvalid},
    {"encode_safe", l_encode_safe},
    {"decode_safe", l_decode_safe},
    {NULL, NULL}
};

LUALIB_API int luaopen_base64(lua_State *L) {
  luaL_register(L, LUA_BASE64LIBNAME, base64lib);
  return 1;
}
