//---------------------------------------------------------------------------------------------------------------------
// MIT License
// 
// Copyright(c) 2021 David "Rez" Graham
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright noticeand this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------------------------------------------------

#pragma once
#include "LuaConfig.h"

#if BLEACHLUA_USING_BLEACH_ENGINE
    #include <BleachUtils/Debug/Debugger.h>

    #define LUA_ASSERT _CHECK
    #define LUA_ASSERT_MSG _CHECK_MSG
    #define LUA_ERROR _ERROR
    #define LUA_INFO _INFO
#else
    #include <assert.h>
    #include <iostream>

    #define LUA_ASSERT assert
    #define LUA_ASSERT_MSG(_expr, _) assert((_expr))
    #define LUA_ERROR(_str) std::cerr << (_str)
    #define LUA_INFO(_str) std::cout << (_str)
#endif

struct lua_State;

#define _SHOW_LUA_ERROR(_state_) \
    LUA_ERROR(luastl::string("Script Syntax Error:\n") + lua_tostring(_state_, -1)); \
    lua_pop(_state_, 1)

#define CHECK_FOR_LUA_ERROR_NO_RETURN(_state_, _error_) \
    if (_error_) \
    { \
        _SHOW_LUA_ERROR(_state_); \
    }

#define CHECK_FOR_LUA_ERROR(_state_, _error_) \
    if (_error_) \
    { \
        _SHOW_LUA_ERROR(_state_); \
        return false; \
    }

#define CHECK_FOR_PCALL_EXCEPTION_NO_RETURN(_state_, _result_) \
    if (_result_ != LUA_OK) \
    { \
        const char* _errorMsg_ = lua_tostring(_state_, -1); \
        if (_errorMsg_) \
            LUA_ERROR(_errorMsg_); \
        else \
            LUA_ERROR("Lua threw unknown exception."); \
    }

#define CHECK_FOR_PCALL_EXCEPTION(_state_, _result_) \
    if (_result_ != LUA_OK) \
    { \
        const char* _errorMsg_ = lua_tostring(_state_, -1); \
        if (_errorMsg_) \
            LUA_ERROR(_errorMsg_); \
        else \
            LUA_ERROR("Lua threw unknown exception."); \
        return false;  /* Note: It is assumed that the stack is cleaned up through StackResetter */ \
    }


namespace BleachLua {

void DumpLuaStack(lua_State* pState, const char* prefix);

#if BLEACHLUA_ENABLE_UNIT_TESTS

class LuaState;

void RunAllUnitTests(LuaState* pState);
void RunLuaVarTests(LuaState* pState);

#endif


}  // end namespace BleachLua
