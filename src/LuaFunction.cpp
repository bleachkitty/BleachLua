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

#include <BleachLua/LuaFunction.h>

namespace BleachLua {

// Has the form of f(errorMsg) -> newErrorMsg
int BaseLuaFunction::OnLuaException(lua_State* pState)
{
    // build up the error message                               //  [error]
    luastl::string finalError = "Lua Exception:\n";
    finalError += lua_tostring(pState, -1);
    lua_pop(pState, 1);                                         //  []

    // add the stack trace
    luaL_traceback(pState, pState, finalError.c_str(), 0);      //  [error+stacktrace]

    return 1;                                                   //  [error+stacktrace]  <-- returned
}

bool BaseLuaFunction::CheckFunctionVar() const
{
    if (!m_functionVar.IsFunction())
    {
        LUA_ERROR("Attempting to call invalid Lua function.");
        return false;
    }
    return true;
}

void LuaFunction<void>::operator()() const
{
    // make sure it's valid
    if (!CheckFunctionVar())
        return;

    lua_State* pState = m_functionVar.GetLuaState()->GetState();

    // push the error handler
    lua_pushcfunction(pState, &BaseLuaFunction::OnLuaException);    //  [exHandler]

    // call the function
    m_functionVar.PushValueToStack();                               //  [exHandler, func]

    // call the function
    const int result = lua_pcall(pState, 0, 0, -2);                 //  [exHandler, error?]
    if (result != LUA_OK)
    {
        LUA_ERROR(lua_tostring(pState, -1));
        lua_pop(pState, 1);                                         //  [exHandler]
    }
                                                                    //  [exHandle]  <-- guaranteed to be just the exception handler here
    lua_pop(pState, 1);                                             //  []
}


}  // end namespace BleachLua
