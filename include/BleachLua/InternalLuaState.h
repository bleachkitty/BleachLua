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
#include "LuaIncludes.h"

//---------------------------------------------------------------------------------------------------------------------
// Important!  This class is incomplete.  Specifically, the template functions GetGlobal() and SetGlobal() are defined 
// in LuaState.h because they rely on StackHelpers.h which in turn relies on the LuaState class.  This creates a 
// circular include between the old LuaState.h and StackHelpers.h.  This is way around it.  In general, you should 
// include LuaState.h rather than this file unless you run into the circular include issue.
//---------------------------------------------------------------------------------------------------------------------

struct lua_State;

namespace BleachLua {

class LuaVar;

class LuaState
{
    lua_State* m_pState;

public:
    // construction
    LuaState() noexcept : m_pState(nullptr) { }
    LuaState(const LuaState& right) = delete;
    LuaState(LuaState&& right) noexcept : m_pState(right.m_pState) { right.m_pState = nullptr; }
    LuaState& operator=(const LuaState& right) = delete;
    LuaState& operator=(LuaState&& right) noexcept { m_pState = right.m_pState; right.m_pState = nullptr; return (*this); }
    ~LuaState();

    // initialization
    bool Init(bool loadAllLibs = true);

    // Lua commands
    LuaVar LoadString(const char* str);
    bool DoString(const char* str) const;
    bool DoFile(const char* path);
    void ClearStack() const;
    void CollectGarbage() const;

    // accessors
    lua_State* GetState() const { return m_pState; }
    LuaVar GetGlobals();
    template <class Type> Type GetGlobal(const char* key);
    template <class Type> void SetGlobal(const char* key, Type&& value);

    // debug
    void DumpStack(const char* prefix = nullptr) const;
};

}  // end namespace BleachLua
