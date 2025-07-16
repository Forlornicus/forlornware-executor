#include <iostream>
#include <Windows.h>
#include <thread>

#include "misc/tp_handler/tp_handler.hpp"

void load()
{
    teleport_handler::initialize();
}

BOOL APIENTRY DllMain( HMODULE mod, DWORD reason, LPVOID res)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        std::thread(load).detach();
    }
    return TRUE;
}

// forlorn was here now im in your closet....
// https://github.com/luadthrow was here, forlorn pls accept my pull request, or you will find me in your closet.....
