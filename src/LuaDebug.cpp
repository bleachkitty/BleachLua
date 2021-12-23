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

#include <BleachLua/LuaDebug.h>

#if BLEACHLUA_DEBUG_MODE

#include <BleachLua/LuaVar.h>
#include <BleachLua/LuaState.h>
#include <BleachLua/LuaFunction.h>

namespace BleachLua::Debug {

luastl::string GetTraceback(BleachLua::LuaState* pState)
{
    LUA_ASSERT(pState);

    luaL_traceback(pState->GetState(), pState->GetState(), nullptr, 0);
    const char* pResult = StackHelpers::GetFromStack<const char*>(pState);

    // eastl::string doesn't handle empty non-null char*'s, so we do this check just in case (even when not 
    // using eastl)
    if (pResult && pResult[0] != '\0')
        return pResult;
    return {};
}

}  // end namespace BleachLua::Debug

#endif  // BLEACHLUA_DEBUG_MODE
