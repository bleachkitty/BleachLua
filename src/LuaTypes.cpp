﻿//---------------------------------------------------------------------------------------------------------------------
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

#include <BleachLua/LuaConfig.h>
#include <BleachLua/LuaTypes.h>
#include <BleachLua/LuaState.h>

namespace BleachLua {

#if BLEACHLUA_CORE_VERSION <= 52
extern void* g_pExtraSpace;
#endif

LuaState* GetCppStateFromCState([[maybe_unused]] lua_State* pState)
{
    LuaState* pCppState = nullptr;
#if BLEACHLUA_CORE_VERSION >= 53
    void* pExtraSpace = lua_getextraspace(pState);
#else
    void* pExtraSpace = &g_pExtraSpace;
#endif
    memcpy(&pCppState, pExtraSpace, sizeof(void*));

    LUA_ASSERT(pCppState);
    return pCppState;
}


}  // end namespace BleachLua
