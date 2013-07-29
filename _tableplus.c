#include "lua.h"
#include "lauxlib.h"


#undef max
#define max(a,b) ((a) > (b) ? (a) : (b))


#if LUA_VERSION_NUM < 502
#define luaL_len(L,n)   luaL_getn(L,(n))
#endif

#define aux_getn(L)  \
  (luaL_checktype(L, 1, LUA_TTABLE), luaL_len(L, 1))


/*
** TABLE -> SCALAR
*/


static int count (lua_State *L) {
  size_t n = 0;
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_pushnil(L);
  while (lua_next(L, 1) != 0) {
    n++;
    lua_pop(L, 1);  /* pop value, leave key */
  }
  lua_pushnumber(L, n);
  return 1;
}


static int sum (lua_State *L) {
  lua_Number acc = 0;
  const int n = aux_getn(L);
  int i;
  for (i = 1; i <= n; i++) {
    lua_rawgeti(L, 1, i);
    acc += luaL_checknumber(L, -1);
    lua_pop(L, 1);
  }
  lua_pushnumber(L, acc);
  return 1;
}


static int product (lua_State *L) {
  lua_Number acc = 1;
  const int n = aux_getn(L);
  int i;
  for (i = 1; i <= n; i++) {
    lua_rawgeti(L, 1, i);
    acc *= luaL_checknumber(L, -1);
    lua_pop(L, 1);
  }
  lua_pushnumber(L, acc);
  return 1;
}


static int foldl (lua_State *L) {
  const int n = aux_getn(L);
  int i = 1;
  luaL_checktype(L, 2, LUA_TFUNCTION);
  if (lua_gettop(L) == 2)  /* no initial value -> first elem of table */
    lua_rawgeti(L, 1, i++);
  else
    lua_settop(L, 3);
  for ( ; i <= n; i++) {
    lua_pushvalue(L, 2);  /* function */
    lua_pushvalue(L, 3);  /* accumulator */
    lua_rawgeti(L, 1, i);  /* table[i] */
    lua_call(L, 2, 1);  /* tmp := function(accumulator, table[i]) */
    lua_replace(L, 3);  /* accumulator := tmp */
  }
  return 1;
}


static int foldr (lua_State *L) {
  int i = aux_getn(L);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  if (lua_gettop(L) == 2)  /* no initial value -> last elem of table */
    lua_rawgeti(L, 1, i--);
  else
    lua_settop(L, 3);
  for ( ; i >= 1; i--) {
    lua_pushvalue(L, 2);  /* function */
    lua_pushvalue(L, 3);  /* accumulator */
    lua_rawgeti(L, 1, i);  /* table[i] */
    lua_call(L, 2, 1);  /* tmp := function(accumulator, table[i]) */
    lua_replace(L, 3);  /* accumulator := tmp */
  }
  return 1;
}


#define checkprednil(L,i) do {                                      \
    luaL_argcheck(L, lua_isnoneornil(L, (i)), (i),                  \
        lua_pushfstring(L, "expected predicate or nothing, got %s", \
                          luaL_typename(L, (i))));                  \
  } while (0)


static int any (lua_State *L) {
  const int n = aux_getn(L);
  int i;
  if (lua_isfunction(L, 2)) {  /* predicate given */
    for (i = 1; i <= n; i++) {
      lua_pushvalue(L, 2);  /* predicate */
      lua_rawgeti(L, 1, i);  /* table[i] */
      lua_call(L, 1, 1);  /* predicate(table[i]) */
      if (lua_toboolean(L, -1)) return 1;
      lua_pop(L, 1);
    }
  }
  else {
    checkprednil(L, 2);
    for (i = 1; i <= n; i++) {
      lua_rawgeti(L, 1, i);  /* table[i] */
      if (lua_toboolean(L, -1)) return 1;
      lua_pop(L, 1);
    }
  }
  lua_pushnil(L);
  return 1;
}


static int all (lua_State *L) {
  const int n = aux_getn(L);
  int i;
  if (lua_isfunction(L, 2)) {  /* predicate given */
    for (i = 1; i <= n; i++) {
      lua_pushvalue(L, 2);  /* predicate */
      lua_rawgeti(L, 1, i);  /* table[i] */
      lua_call(L, 1, 1);  /* predicate(table[i]) */
      if (!lua_toboolean(L, -1)) return 1;
      lua_pop(L, 1);
    }
  }
  else {
    checkprednil(L, 2);
    for (i = 1; i <= n; i++) {
      lua_rawgeti(L, 1, i);  /* table[i] */
      if (!lua_toboolean(L, -1)) return 1;
      lua_pop(L, 1);
    }
  }
  lua_pushvalue(L, 1);  /* push table as truth value */
  return 1;
}


/*
** TABLE -> TABLE
*/


static int copy (lua_State *L) {
  const int n = aux_getn(L);
  int i;
  lua_settop(L, 1);
  lua_createtable(L, n, 0);  /* copy of table */
  for (i = 1; i <= n; i++) {
    lua_rawgeti(L, 1, i);
    lua_rawseti(L, 2, i);  /* copy[i] := table[i] */
  }
  return 1;
}


static int slice (lua_State *L) {
  const int n = aux_getn(L),
            first = luaL_optint(L, 2, 1),
            last = luaL_optint(L, 3, n),
            slice_n = last - first + 1;
  int i, k;
  if (n == 0 || slice_n <= 0 || n < slice_n) {
    lua_newtable(L);  /* empty table */
    return 1;
  }
  else if (n == slice_n) {  /* copy */
    lua_pushcfunction(L, copy);
    lua_pushvalue(L, 1);
    lua_call(L, 1, 1);
    return 1;
  }
  lua_settop(L, 1);
  for (k = 0, i = first; i <= last; i++) {
    lua_rawgeti(L, 1, i);
    lua_rawseti(L, 2, ++k);
  }
  return 1;
}


static int map (lua_State *L) {
  const int n = aux_getn(L);
  int i;
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_settop(L, 2);
  lua_createtable(L, n, 0);  /* newtab := map(oldtab, func) */
  for (i = 1; i <= n; i++) {
    lua_pushvalue(L, 2);  /* function */
    lua_rawgeti(L, 1, i);  /* oldtab[i] */
    lua_call(L, 1, 1);  /* function(oldtab[i]) */
    lua_rawseti(L, 3, i);  /* newtab[i] := function(oldtab[i]) */
  }
  return 1;
}


static int filter (lua_State *L) {
  const int n = aux_getn(L);
  int i, newtabindex = 0;
  lua_settop(L, 2);
  lua_createtable(L, n, 0);  /* newtab := filter(oldtab, pred) */
  if (lua_isfunction(L, 2)) {  /* predicate given */
    for (i = 1; i <= n; i++) {
      lua_rawgeti(L, 1, i);  /* oldtab[i] */
      lua_pushvalue(L, 2);  /* predicate */
      lua_pushvalue(L, -2);  /* oldtab[i] (copy) */
      lua_call(L, 1, 1);  /* predicate(oldtab[i]) */
      if (lua_toboolean(L, -1)) {
        lua_pop(L, 1);
        lua_rawseti(L, 3, ++newtabindex);
      }
      else lua_pop(L, 2);  /* pop oldtab[i] */
    }
  }
  else {
    checkprednil(L, 2);
    for (i = 1; i <= n; i++) {
      lua_rawgeti(L, 1, i);  /* oldtab[i] */
      if (lua_toboolean(L, -1))
        lua_rawseti(L, 3, ++newtabindex);
      else lua_pop(L, 1);  /* pop oldtab[i] */
    }
  }
  return 1;
}


static int reversed (lua_State *L) {
  const int n = aux_getn(L);
  int i, r;
  lua_settop(L, 1);
  lua_createtable(L, n, 0);  /* reversed table */
  for (i = n, r = 1; i >= 1; i--, r++) {
    lua_rawgeti(L, 1, i);
    lua_rawseti(L, 2, r);
  }
  return 1;
}


/*
** TABLE -> ITERATOR
*/


static int imap_aux (lua_State *L) {
  const int i = luaL_checkint(L, 2) + 1;
  lua_pushinteger(L, i);  /* index */
  lua_pushvalue(L, lua_upvalueindex(1));  /* mapper */
  lua_rawgeti(L, 1, i);  /* table[index] */
  if (lua_isnil(L, -1)) return 1;  /* break iterator */
  lua_call(L, 1, 1);  /* value := mapper(table[index]) */
  return 2;  /* index, value */
}


static int imap (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_settop(L, 2);
  lua_pushcclosure(L, imap_aux, 1);  /* imap_aux(mapper) => map closure */
  lua_pushvalue(L, 1);  /* table */
  lua_pushinteger(L, 0);  /* index 0 */
  return 3;
}


static int ireverse_aux (lua_State *L) {
  const int i = luaL_checkint(L, 2) - 1;
  lua_pushinteger(L, i);  /* index */
  lua_rawgeti(L, 1, i);  /* table[index] */
  return lua_isnil(L, -1) ? 1 : 2;
}


static int ireverse (lua_State *L) {
  const int n = aux_getn(L);
  lua_pushcfunction(L, ireverse_aux);  /* function */
  lua_pushvalue(L, 1);  /* table */
  lua_pushinteger(L, n+1);  /* one past last index */
  return 3;
}


#define TABLE   lua_upvalueindex(1)
#define PRED    lua_upvalueindex(2)
#define TLEN    lua_upvalueindex(3)
#define INTIDX  lua_upvalueindex(4)
#define EXTIDX  lua_upvalueindex(5)


#define update(L,upvi,val) \
  (lua_pushinteger(L, (val)), lua_replace(L, (upvi)))


static int ifilter_pred_aux (lua_State *L) {
  const int n = lua_tointeger(L, TLEN);
  int int_idx = lua_tointeger(L, INTIDX);
  int i, has_next = 0;
  for (i = int_idx; i <= n; i++) {
    lua_rawgeti(L, TABLE, i);
    lua_pushvalue(L, PRED);
    lua_pushvalue(L, -2);  /* copy t[i] */
    lua_call(L, 1, 1);
    if (lua_toboolean(L, -1)) {
      lua_pop(L, 1);  /* pop result */
      has_next = 1;
      break;
    }
    else lua_pop(L, 2);  /* pop result & t[i] */
  }
  update(L, INTIDX, i + 1);
  if (has_next) {
    int ext_idx = lua_tointeger(L, EXTIDX);
    update(L, EXTIDX, ext_idx + 1);
    lua_pushinteger(L, ext_idx);  /* key */
    lua_pushvalue(L, -2);  /* value */
    return 2;
  }
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int ifilter_plain_aux (lua_State *L) {
  const int n = lua_tointeger(L, TLEN);
  int int_idx = lua_tointeger(L, INTIDX);
  int i, has_next = 0;
  for (i = int_idx; i <= n; i++) {
    lua_rawgeti(L, TABLE, i);
    if (lua_toboolean(L, -1)) {
      has_next = 1;
      break;
    }
    else lua_pop(L, 1);
  }
  update(L, INTIDX, i + 1);
  if (has_next) {
    int ext_idx = lua_tointeger(L, EXTIDX);
    update(L, EXTIDX, ext_idx + 1);
    lua_pushinteger(L, ext_idx);  /* key */
    lua_pushvalue(L, -2);  /* value */
    return 2;
  }
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int ifilter (lua_State *L) {
  lua_CFunction aux;
  const int n = aux_getn(L);
  if (lua_isfunction(L, 2)) {
    aux = ifilter_pred_aux;
  }
  else {
    checkprednil(L, 2);
    aux = ifilter_plain_aux;
  }
  lua_settop(L, 2);  /* table, predicate */
  lua_pushinteger(L, n);  /* table length */
  lua_pushinteger(L, 1);  /* internal index */
  lua_pushinteger(L, 1);  /* external index */
  lua_pushcclosure(L, aux, 5);
  return 1;
}


/*
** FORWARD AND BACKWARD COMPATIBILITY TO/FROM LUA 5.2
*/


#if LUA_VERSION_NUM < 502  /* { */


static int pack (lua_State *L) {
  const int n = lua_gettop(L);
  int i;
  luaL_checkstack(L, (n+2), "too many values to pack");
  lua_createtable(L, n, 1);
  lua_pushinteger(L, n);
  lua_setfield(L, -2, "n");  /* set length as field "n" */
  for (i = 1; i <= n; i++) {
    lua_pushvalue(L, i);
    lua_rawseti(L, -2, i);
  }
  return 1;
}


static int unpack (lua_State *L) {
  const int n = aux_getn(L);
  int i;
  if (n == 0) return 0;
  lua_settop(L, 1);
  luaL_checkstack(L, (n+1), "too many values to unpack");
  for (i = 1; i <= n; i++)
    lua_rawgeti(L, 1, i);
  return n;
}


#else  /* }{ LUA_VERSION_NUM >= 502 */


static int foreachi (lua_State *L) {
  const int n = aux_getn(L);
  int i;
  luaL_checktype(L, 2, LUA_TFUNCTION);
  for (i=1; i <= n; i++) {
    lua_pushvalue(L, 2);  /* function */
    lua_pushinteger(L, i);  /* 1st argument */
    lua_rawgeti(L, 1, i);  /* 2nd argument */
    lua_call(L, 2, 1);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 1);  /* remove nil result */
  }
  return 0;
}


static int foreach (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  lua_pushnil(L);  /* first key */
  while (lua_next(L, 1)) {
    lua_pushvalue(L, 2);  /* function */
    lua_pushvalue(L, -3);  /* key */
    lua_pushvalue(L, -3);  /* value */
    lua_call(L, 2, 1);
    if (!lua_isnil(L, -1))
      return 1;
    lua_pop(L, 2);  /* remove value and result */
  }
  return 0;
}


#endif  /* } LUA_VERSION_NUM <=> 502 */


/*
** MODULE SETUP
*/


static const luaL_Reg lib[] = {
  {"count", count},
  {"sum", sum},
  {"product", product},
  {"foldl", foldl},
  {"foldr", foldr},
  {"any", any},
  {"all", all},
  {"copy", copy},
  {"slice", slice},
  {"map", map},
  {"filter", filter},
  {"reversed", reversed},
  {"imap", imap},
  {"ireverse", ireverse},
  {"ifilter", ifilter},
#if LUA_VERSION_NUM < 502
  {"pack", pack},
  {"unpack", unpack},
#else
  {"foreach", foreach},
  {"foreachi", foreachi},
#endif
  {NULL, NULL}
};


static void push_table_lib (lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  luaL_checktype(L, -1, LUA_TTABLE);
  lua_getfield(L, -1, "table");
  luaL_checktype(L, -1, LUA_TTABLE);
}


#if LUA_VERSION_NUM == 502
#define luaL_register(L,n,l) luaL_setfuncs(L,l,0)
#endif


int luaopen__tableplus (lua_State *L) {
  push_table_lib(L);
  luaL_register(L, NULL, lib);
  return 1;
}
