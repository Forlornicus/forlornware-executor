#include "script_library.hpp"
#include <windows.h>    
#include <string>   


int identifyexecutor(lua_State* L)
{
    lua_pushstring(L, "ForlornWare");
    lua_pushstring(L, "1.0.0");
    return 2;
}

int getgenv(lua_State* L)
{
    lua_pushvalue(L, LUA_ENVIRONINDEX);
    return 1;
}

int setclipboard(lua_State* L)
{
    luaL_checktype(L, 1, LUA_TSTRING);
    std::string content = lua_tostring(L, 1);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, content.size() + 1);
    char* pMem = (char*)GlobalLock(hMem);
    memcpy(pMem, content.data(), content.size());
    pMem[content.size()] = '\0';  
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();
    return 0;
}


int isrbxactive(lua_State* L)
{
    
  HWND window;
    window = FindWindowA(NULL, "Roblox");
    lua_pushboolean(L, GetForegroundWindow() == window);
    return 1;

}


void script_library::initialize(lua_State* L)
{
    register_env_functions(L,
        {
            {"identifyexecutor", identifyexecutor},
            {"setclipboard", setclipboard},
             {"isrbxactive", isrbxactive},
             
            {"getgenv", getgenv},
            {nullptr, nullptr}
        });
}
