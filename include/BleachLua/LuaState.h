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

#include "InternalLuaState.h"
#include "StackHelpers.h"
#include "LuaError.h"

// LuaVar.h is required to be included before LuaState.h.  This is a workaround so the user doesn't have to care.
// The downside is that LuaVar.h is a very large file that's not technically needed here unless both are being used, 
// so this adds a possibly unnecessary large compile-time cost.
// 
// TODO: Fix this dependency.  This should be trivial in C++ 20 with modules, but there is likely a better way to 
// structure things now to get around this.
#include "LuaVar.h"

//---------------------------------------------------------------------------------------------------------------------
// Important!  This file defines the template functions GetGlobal() and SetGlobal() from the LuaState class.  This 
// gets around a circular dependency where we essentially need StackHelpers.h to be included between this code and 
// the class definition for LuaState.h.  See comment in InternalLuaState.h for details.  In general, you should 
// include this file rather than InternalLuaState.h.
//---------------------------------------------------------------------------------------------------------------------

namespace BleachLua {

template <class Type>
Type LuaState::GetGlobal(const char* key)
{
    // check for a bad key
    if (!key)
    {
        LUA_ERROR("nullptr key passed to GetGlobal().");
        return StackHelpers::template GetDefault<Type>();  // template keyword is needed here for template disambiguation: https://en.cppreference.com/w/cpp/language/dependent_name
    }

    // grab the global and return it
    lua_getglobal(m_pState, key);  // push the global onto the stack
    Type result = StackHelpers::template Get<Type>(this);  // template keyword is needed here for template disambiguation: https://en.cppreference.com/w/cpp/language/dependent_name
    lua_pop(m_pState, 1);  // pop the stack so it's in the same state

    return result;
}

template <class Type>
void LuaState::SetGlobal(const char* key, Type&& value)
{
    StackHelpers::Push(this, std::forward<Type>(value));
    lua_setglobal(m_pState, key);
}

}
