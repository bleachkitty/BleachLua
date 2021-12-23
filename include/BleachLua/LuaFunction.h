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
#include "StackHelpers.h"
#include "LuaVar.h"
#include "LuaState.h"

namespace BleachLua {

//---------------------------------------------------------------------------------------------------------------------
// Base Lua Function.  This is an internal class; use the template class below.
//---------------------------------------------------------------------------------------------------------------------
class BaseLuaFunction
{
protected:
    LuaVar m_functionVar;

public:
    BaseLuaFunction(const LuaVar& functionVar) : m_functionVar(functionVar) { LUA_ASSERT(m_functionVar.IsFunction()); }
    BaseLuaFunction(LuaVar&& functionVar) : m_functionVar(std::move(functionVar)) { LUA_ASSERT(m_functionVar.IsFunction()); }

protected:
    template <class Arg, class... RemainingArgs> void PushArguments(Arg arg, RemainingArgs... remainingArgs) const;
    template <class Arg> void PushArguments(Arg arg) const;

    bool CheckFunctionVar() const;
    static int OnLuaException(lua_State* pState);
    static constexpr int GetExceptionHandlerStackOffset(size_t numArgs) { return -(static_cast<int>(numArgs + 2)); }
};


template <class Arg, class ... RemainingArgs>
void BaseLuaFunction::PushArguments(Arg arg, RemainingArgs... remainingArgs) const
{
    PushArguments(arg);
    PushArguments(remainingArgs...);
}

template <class Arg>
void BaseLuaFunction::PushArguments(Arg arg) const
{
    StackHelpers::Push<Arg>(m_functionVar.GetLuaState(), arg);
}


//---------------------------------------------------------------------------------------------------------------------
// Lua Function.  This defines a single, callable Lua function.
//---------------------------------------------------------------------------------------------------------------------
template <class RetType>
class LuaFunction : public BaseLuaFunction
{
public:
    explicit LuaFunction(const LuaVar& functionVar) : BaseLuaFunction(functionVar) { }
    LuaFunction(LuaVar&& functionVar) : BaseLuaFunction(std::move(functionVar)) { }

    template <class... Args> RetType operator()(Args... args) const;
    RetType operator()() const;
};

template <class RetType>
template <class... Args>
RetType LuaFunction<RetType>::operator()(Args... args) const
{
    // make sure it's valid
    if (!CheckFunctionVar())
        return StackHelpers::GetDefault<RetType>();

    lua_State* pState = m_functionVar.GetLuaState()->GetState();
    StackHelpers::StackResetter resetter(pState, lua_gettop(pState));

    // push the error handler
    lua_pushcfunction(pState, &BaseLuaFunction::OnLuaException);                                                //  [exHandler]

    // push the function to the stack
    m_functionVar.PushValueToStack();                                                                           //  [exHandler, func]

    // push the params
    PushArguments(args...);                                                                                     //  [exHandler, func, args...]
    const int result = lua_pcall(pState, sizeof...(Args), 1, GetExceptionHandlerStackOffset(sizeof...(Args)));  //  [exHandler, ret|error]
    if (result != LUA_OK)
    {
        LUA_ERROR(lua_tostring(pState, -1));
        return StackHelpers::GetDefault<RetType>();                                                             //  []  <-- from StackResetter
    }

    // get the return                                                                                           //  [exHandler, ret]  <-- guaranteed to be the ret
    return StackHelpers::Get<RetType>(m_functionVar.GetLuaState());                                             //  []  <-- from StackResetter
}

template <class RetType>
RetType LuaFunction<RetType>::operator()() const
{
    // make sure it's valid
    if (!CheckFunctionVar())
        return StackHelpers::Get<RetType>(m_functionVar.GetLuaState());

    lua_State* pState = m_functionVar.GetLuaState()->GetState();
    StackHelpers::StackResetter resetter(pState, lua_gettop(pState));

    // push the error handler
    lua_pushcfunction(pState, &BaseLuaFunction::OnLuaException);                        //  [exHandler]

    // call the function
    m_functionVar.PushValueToStack();                                                   //  [exHandler, func]
    const int result = lua_pcall(pState, 0, 1, -2);                                     //  [exHandler, ret|error]
    if (result != LUA_OK)
    {
        LUA_ERROR(lua_tostring(pState, -1));
        return StackHelpers::GetDefault<RetType>();                                     //  []  <-- from StackResetter
    }

    // get the return
    return StackHelpers::Get<RetType>(m_functionVar.GetLuaState());                     //  []  <-- from StackResetter
}

//---------------------------------------------------------------------------------------------------------------------
// This specialization of LuaFunction is for no return type.
//---------------------------------------------------------------------------------------------------------------------
template <>
class LuaFunction<void> : public BaseLuaFunction
{
public:
    LuaFunction(const LuaVar& functionVar) : BaseLuaFunction(functionVar) { }
    LuaFunction(LuaVar&& functionVar) : BaseLuaFunction(std::move(functionVar)) { }

    template <class... Args> void operator()(Args... args) const;
    void operator()() const;
};

template <class... Args>
void LuaFunction<void>::operator()(Args... args) const
{
    // make sure it's valid
    if (!CheckFunctionVar())
        return;

    lua_State* pState = m_functionVar.GetLuaState()->GetState();
    StackHelpers::StackResetter resetter(pState, lua_gettop(pState));

    // push the error handler
    lua_pushcfunction(pState, &BaseLuaFunction::OnLuaException);                                                //  [exHandler]
                                                                                                                
    // push the function to the stack
    m_functionVar.PushValueToStack();                                                                           //  [exhandler, func]

    // push the params
    PushArguments(args...);                                                                                     //  [exHandler, func, args...]
    const int result = lua_pcall(pState, sizeof...(Args), 0, GetExceptionHandlerStackOffset(sizeof...(Args)));  //  [exHandler, ret|error]
    if (result != LUA_OK)
        LUA_ERROR(lua_tostring(pState, -1));
}                                                                                                               //  []  <-- from StackResetter


}  // end BleachLua
