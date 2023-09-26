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

#include "TestApp.h"
#include <assert.h>
#include <BleachLua/LuaFunction.h>
#include <BleachLua/TableIterator.h>

using namespace BleachLua;

TestApp::~TestApp()
{
    // This sets all the bound functions to nil in the Lua state.  This isn't necessary if you're tearing down the 
    // game, which means I don't actually need to do it here.  I'm including it to show you how to make Lua "forget" 
    // about bound functions.
    LuaVar globals = m_luaState.GetGlobals();
    globals.SetTableNil("FastSquare");
    globals.SetTableNil("PrintString");
    globals.SetTableNil("SumValues");
}

bool TestApp::Init()
{
    // Init() must be called before using Lua.  It will return true on success, false on failure.  By default, it 
    // will load all the standard Lua libraries, but you can skip that.  If you do, any libs you want to use will 
    // need to be loaded manually.
    if (!m_luaState.Init())
        return false;

    // This is a convenience call to set the default Lua state.  Most of the time, you will work with a single state, 
    // so calling this static function allows all LuaVar's created to come from this same state.
    LuaVar::SetDefaultLuaState(&m_luaState);

    // We need to bind all the functions we're going to call from Lua.  First, we get the Lua table where all globals 
    // are stored.  This could be any table, but we're using the globals table for simplicity.
    LuaVar globals = m_luaState.GetGlobals();

    // Now we bind each function.  Static & free functions are easy; just bind them directly.
    globals.BindFunction("FastSquare", &TestApp::FastSquare);  

    // Member functions are nearly as easy, they just have an extra parameter to bind the 'this' pointer.  Note that 
    // Lua has no way of knowing whether or not this pointer is valid, so great care must be used here to not call 
    // into a dangling pointer.
    globals.BindFunction("PrintString", this, &TestApp::PrintString);

    // Called by the table test.
    globals.BindFunction("SumValues", &TestApp::SumValues);

    return true;
}

void TestApp::RunStateExample()
{
    std::cout << "\n===== Basic Examples =====\n";

    // Executing a string as Lua code.
    m_luaState.DoString("print('Hello World')");

    // Failing to execute a string.  Uncomment this if you want to see how it fails.  The error that's logged 
    // is whatever Lua spits out.
    //m_luaState.DoString("ThisIsntARealFunction()");

    // run a file
    if (!m_luaState.DoFile("TestScripts/Test.lua"))
        std::cerr << "Could't find Test.lua\n";
}

void TestApp::CallLuaFunctionFromCpp()
{
    std::cout << "\n===== Calling Into Lua =====\n";

    // load the test file
    if (!m_luaState.DoFile("TestScripts/CallIntoLuaExample.lua"))
    {
        std::cerr << "Could't find CallIntoLuaExample.lua\n";
        return;
    }

    // SimpleTest()
    LuaFunction<void> simpleTest = m_luaState.GetGlobal<LuaVar>("SimpleTest");
    simpleTest();

    // TestWithParams()
    LuaFunction<void> testWithParams = m_luaState.GetGlobal<LuaVar>("TestWithParams");
    testWithParams(3, "Cat");  // values can be anything Lua accepts (any integral, any floating-point type, C-style string, bool, any LuaVar, etc)

    // TestWithReturn()
    // Note the template param is now int, which is the return type from Lua.  In order to keep C++-like syntax, any 
    // additional return values from Lua are ignored.  If you need multiple values to be returned from Lua, return 
    // a table instead.
    LuaFunction<int> testWithReturn = m_luaState.GetGlobal<LuaVar>("TestWithReturn");
    const int result = testWithReturn(5);
    std::cout << "Result: " << result << "\n";
}

void TestApp::CallCppFunctionsFromLua()
{
    std::cout << "\n===== Calling Into C++ =====\n";

    // load the test file
    if (!m_luaState.DoFile("TestScripts/CallIntoCppExample.lua"))
    {
        std::cerr << "Could't find CallIntoCppExample.lua\n";
        return;
    }

    // call the Lua function that will in turn call the bound C++ functions
    LuaFunction<void> callCppFunctions = m_luaState.GetGlobal<LuaVar>("CallCppFunctions");
    callCppFunctions();
}

void TestApp::FunWithTables()
{
    std::cout << "\n===== Fun With Tables =====\n";

    // load the test file
    if (!m_luaState.DoFile("TestScripts/TablesExample.lua"))
    {
        std::cerr << "Could't find TablesExample.lua\n";
        return;
    }

    // call the Lua function that will send a table as an array, which we will sum
    LuaFunction<void> luaSendsMeATable = m_luaState.GetGlobal<LuaVar>("LuaSendsMeATable");
    luaSendsMeATable();

    // create a new empty table
    LuaVar animals;  // Note: If we hadn't called SetDefaultLuaState() in TestApp::Init(), we'd have to pass a pointer to the LuaState here
    animals.CreateNewTable();  // you can pass params to tell Lua to pre-allocate storage if you want

    // Add some lives.  Note that the SetTableXXX() and GetTableXXX() functions are for setting and getting values 
    // set values on or from a table.  By contrast, the SetXXX() and GetXXX() functions are for when the var is a 
    // scalar.
    animals.SetTableInteger("cat", 9);
    animals.SetTableInteger("dog", 5);
    animals.SetTableInteger("rat", 3);

    // The above sequence is the same as the following lua code:
    //      animals = {}
    //      animals.cat = 9
    //      animals.dog = 5
    //      animals.rat = 3

    // print the table with a call to Lua, passing in the table we created
    LuaFunction<void> printTable = m_luaState.GetGlobal<LuaVar>("PrintTable");
    printTable(animals);
}

int TestApp::FastSquare(int val)
{
    return val * val;
}

void TestApp::PrintString(const char* str) const
{
    std::cout << str << "\n";
}

int TestApp::SumValues(LuaVar values)
{
    // make sure we have a table
    if (!values.IsTable())
        return 0;

    // Loop through the table using a structured binding and range-based for loop.  There are other methods 
    // as well, like using an index.  Note that key and val are both LuaVar's so they can be anything.  The 
    // hash table section works the same way, so the loop would look exacly the same.
    int total = 0;
    for (const auto& [key, val] : values)
    {
        assert(val.IsInteger());  // make sure it's an int; LuaVar can be anything
        total += val.GetInteger<int>();  // interpret the LuaVar as an int
    }

    // This version uses a key/value pair object, similar to unorder_map's, in case you don't want to use the 
    // structured binding approach.
    //for (const auto& keyValPair : values)
    //{
    //    assert(keyValPair.GetKey().IsInteger());  // make sure it's an int; LuaVar can be anything
    //    total += keyValPair.GetValue().GetInteger();  // interpret the LuaVar as an int
    //}

    return total;
}
