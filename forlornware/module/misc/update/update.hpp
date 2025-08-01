#pragma once
#include <cstdint>
#include <string>
#include <Windows.h>
#include <iostream>

#include "enchelper/enchelper.hpp"

struct lua_State;

#define rebase(x) x + (uintptr_t)GetModuleHandle(nullptr)
#define rebase_hyperion(x) x + (uintptr_t)GetModuleHandleA("RobloxPlayerBeta.dll")
    
// updated for version-b8550645b8834e8a
// offsets from discord.gg/6xFacqR2
namespace update
{
    // you only need to update this if you are calling the patch cfg function in entry.cpp
    const uintptr_t bitmap = rebase_hyperion(0x2B74D0);

    namespace roblox
    {
        const uintptr_t print = rebase(0x1516A40);
        const uintptr_t luad_throw = rebase(0x2709910);
        const uintptr_t get_lua_state = rebase(0xBC3D70);
        const uintptr_t request_code = rebase(0x1065BA0);
    }

    namespace lua
    {
        const uintptr_t luao_nilobject = rebase(0x4CCEFF8);
        const uintptr_t luau_execute = rebase(0x273C6C0);
        const uintptr_t luah_dummynode = rebase(0x4CCE728);
        const uintptr_t opcode_lookup = rebase(0x4DBE9A0); //gay
    }

    namespace offsets
    {
        namespace datamodel
        {
            const uintptr_t fake_datamodel = rebase(0x68D7308); //gay
            const uintptr_t fake_datamodel_to_datamodel = 0x1C0;
            const uintptr_t script_context = 0x3C0;
            const uintptr_t game_loaded = 0x680;
        }

        namespace instance
        {
            const uintptr_t name = 0x78;
            const uintptr_t children = 0x80;
        }
    }
}

namespace roblox
{
    using print_func_t = int(__fastcall*)(int64_t, const char*, ...);
    inline print_func_t r_print = reinterpret_cast<print_func_t>(update::roblox::print);

    using decrypt_state_t = int64_t(__fastcall*)(int64_t, uint64_t*, uint64_t*);
    inline decrypt_state_t get_state = reinterpret_cast<decrypt_state_t>(update::roblox::get_lua_state);

    using luad_throw_t = void(__fastcall*)(lua_State*, int);
    inline luad_throw_t luad_throw = reinterpret_cast<luad_throw_t>(update::roblox::luad_throw);

    using request_code_t = uintptr_t(__fastcall*)(uintptr_t, uintptr_t);
    inline request_code_t request_code = reinterpret_cast<request_code_t>(update::roblox::request_code);
}

#define LUAU_COMMA_SEP ,
#define LUAU_SEMICOLON_SEP ;

#define LUAU_SHUFFLE3(s, a1, a2, a3) a3 s a2 s a1
#define LUAU_SHUFFLE4(s, a1, a2, a3, a4) a3 s a2 s a4 s a1
#define LUAU_SHUFFLE5(s, a1, a2, a3, a4, a5) a3 s a4 s a2 s a5 s a1
#define LUAU_SHUFFLE6(s, a1, a2, a3, a4, a5, a6) a2 s a4 s a3 s a1 s a5 s a6
#define LUAU_SHUFFLE7(s, a1, a2, a3, a4, a5, a6, a7) a4 s a5 s a1 s a7 s a2 s a3 s a6
#define LUAU_SHUFFLE8(s, a1, a2, a3, a4, a5, a6, a7, a8) a3 s a2 s a4 s a8 s a6 s a5 s a7 s a1
#define LUAU_SHUFFLE9(s, a1, a2, a3, a4, a5, a6, a7, a8, a9) a5 s a1 s a7 s a3 s a4 s a9 s a2 s a6 s a8

#define PROTO_MEMBER2_ENC     vmval2
#define PROTO_DEBUGISN_ENC    vmval4
#define PROTO_TYPEINFO_ENC    vmval1
#define PROTO_DEBUGNAME_ENC   vmval3
#define PROTO_MEMBER1_ENC     vmval0

#define LSTATE_STACKSIZE_ENC  vmval1
#define LSTATE_GLOBAL_ENC     vmval0

#define CLOSURE_CONT_ENC      vmval4
#define CLOSURE_DEBUGNAME_ENC vmval2
#define CLOSURE_FUNC_ENC      vmval0

#define TSTRING_HASH_ENC      vmval3
#define TSTRING_LEN_ENC       vmval0

#define TABLE_MEMBER_ENC      vmval0
#define TABLE_META_ENC        vmval0

#define UDATA_META_ENC        vmval4

#define GSTATE_TTNAME_ENC     vmval0
#define GSTATE_TMNAME_ENC     vmval0
