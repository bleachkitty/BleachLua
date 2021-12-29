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

#include <BleachLua/LuaState.h>

class TestApp
{
    BleachLua::LuaState m_luaState;

public:
    ~TestApp();
    bool Init();

    void RunStateExample();
    void CallLuaFunctionFromCpp();
    void CallCppFunctionsFromLua();
    void FunWithTables();

private:
    // called from Lua as part of the call-into-C++ example
    static int FastSquare(int val);
    void PrintString(const char* str) const;  // const and non-const member functions are both allowed

    // Called from Lua as part of the table example.  Important!  Note that we are passing LuaVar by value here.  The 
    // bound C++ function must have the LuaVar passed by value, *not* const ref.  Anything can (and should) use a const 
    // ref, but the function actually bound to Lua can't.  The reason is because the variable needs to be added to the 
    // Lua registry, which causes Lua to increment it's internal ref count and will prevent the garbage collector from 
    // collecting this variable until the LuaVar desructor is run.
    static int SumValues(BleachLua::LuaVar values);
};

