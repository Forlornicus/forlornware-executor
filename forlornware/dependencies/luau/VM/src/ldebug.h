// This file is part of the Luau programming language and is licensed under MIT License; see LICENSE.txt for details
// This code is based on Lua 5.x implementation licensed under MIT License; see lua_LICENSE.txt for details
#pragma once

#include "lstate.h"

#define pcRel(pc, p) ((pc) && (pc) != (p)->code ? cast_to(int, (pc) - (p)->code) - 1 : 0)
// TODO: remove with FFlagLuauCurrentLineBounds
#define pcRel_DEPRECATED(pc, p) ((pc) ? cast_to(int, (pc) - (p)->code) - 1 : 0)

#define luaG_typeerror(L, o, opname) luaG_typeerrorL(L, o, opname)
#define luaG_forerror(L, o, what) luaG_forerrorL(L, o, what)
#define luaG_runerror(L, fmt, ...) luaG_runerrorL(L, fmt, ##__VA_ARGS__)

#define LUA_MEMERRMSG "not enough memory"
#define LUA_ERRERRMSG "error in error handling"

LUAI_FUNC void luaG_typeerrorL(lua_State* L, const TValue* o, const char* opname);
LUAI_FUNC void luaG_forerrorL(lua_State* L, const TValue* o, const char* what);
LUAI_FUNC void luaG_concaterror(lua_State* L, StkId p1, StkId p2);
LUAI_FUNC void luaG_aritherror(lua_State* L, const TValue* p1, const TValue* p2, TMS op);
LUAI_FUNC void luaG_ordererror(lua_State* L, const TValue* p1, const TValue* p2, TMS op);
LUAI_FUNC void luaG_indexerror(lua_State* L, const TValue* p1, const TValue* p2);
LUAI_FUNC void luaG_methoderror(lua_State* L, const TValue* p1, const TString* p2);
LUAI_FUNC void luaG_readonlyerror(lua_State* L);

LUAI_FUNC void luaG_runerrorL(lua_State* L, const char* fmt, ...);
LUAI_FUNC void luaG_pusherror(lua_State* L, const char* error);

LUAI_FUNC void luaG_breakpoint(lua_State* L, Proto* p, int line, bool enable);
LUAI_FUNC bool luaG_onbreak(lua_State* L);

LUAI_FUNC int luaG_getline(Proto* p, int pc);

LUAI_FUNC int luaG_isnative(lua_State* L, int level);
LUAI_FUNC int luaG_hasnative(lua_State* L, int level);
