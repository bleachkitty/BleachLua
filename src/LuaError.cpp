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

#include <BleachLua/LuaError.h>
#include <BleachLua/LuaIncludes.h>
#include <BleachLua/LuaStl.h>
#include <BleachLua/LuaStringUtils.h>

#if BLEACHLUA_ENABLE_UNIT_TESTS
    #include <BleachLua/LuaState.h>
    #include <BleachLua/LuaVar.h>
#endif

namespace BleachLua {

void DumpLuaStack(lua_State* pState, const char* prefix)
{
    const int top = lua_gettop(pState);
    luastl::string buffer;

    if (prefix)
        LUA_INFO(prefix);
    buffer += "[ ";

    for (int i = 1; i <= top; ++i)
    {
        const int type = lua_type(pState, i);
        switch (type)
        {
            case LUA_TSTRING:
                buffer += "`" + luastl::string(lua_tostring(pState, i)) + "'";
                break;

            case LUA_TBOOLEAN:
                buffer += lua_toboolean(pState, i) ? "true" : "false";
                break;

            case LUA_TNUMBER:
                buffer += TO_STRING(lua_tonumber(pState, i));
                break;

            default:
                buffer += luastl::string(lua_typename(pState, type));
                break;

        }
        buffer += ", ";  // put a separator
    }
    buffer += "]";
    LUA_INFO(buffer);
    LUA_INFO("---------------------------------------------------");
}

#if BLEACHLUA_ENABLE_UNIT_TESTS

void RunAllUnitTests(LuaState* pState)
{
    RunLuaVarTests(pState);
}

void RunLuaVarTests(LuaState* pState)
{
    LuaVar var(pState);
    var.SetBool(true);
}

#endif

}  // end namespace BleachLua
