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

#include <BleachLua/InternalLuaState.h>
#include <BleachLua/LuaError.h>
#include <BleachLua/LuaVar.h>
#include <BleachLua/LuaStl.h>

namespace BleachLua {

#if BLEACHLUA_CORE_VERSION <= 52
// Lua 5.3 added the concept of "extra space", which is a chunk of data associated with each Lua state that could be 
// used for anything the application wants.  We use it to store the Bleach LuaState pointer.  In versions prior to 
// 5.3, we use this sad global variable instead.  The big ramification of this is that we can only have a single Lua 
// state.
void* g_pExtraSpace = nullptr;
#endif

static int OnLuaException(lua_State* pState)
{
    // build up the error message                               //  [error]
    luastl::string finalError = "Lua Exception:\n";
    finalError += lua_tostring(pState, -1);
    lua_pop(pState, 1);                                         //  []

                                                                // add the stack trace
    luaL_traceback(pState, pState, finalError.c_str(), 0);      //  [error+stacktrace]

    const char* msg = lua_tostring(pState, -1);
    if (msg)
        LUA_ERROR(msg);
    else
        LUA_ERROR("Luw threw an unknown exception.");

    return 1;                                                   //  [error+stacktrace]  <-- returned
}

LuaState::~LuaState()
{
    if (m_pState)
        lua_close(m_pState);
}

bool LuaState::Init(bool loadAllLibs /*= true*/)
{
    // initialize lua
    m_pState = luaL_newstate();
    if (!m_pState)
    {
        LUA_ERROR("Couldn't create Lua state.");
        return false;
    }

    // open the libs if necessary
    if (loadAllLibs)
        luaL_openlibs(m_pState);

    // Set Lua's extra space to the value of the C++ state object so we can always get the C++ state when we only 
    // have the lua_State.  This is necessary for bound function calls.
#if BLEACHLUA_CORE_VERSION >= 53
    void* pExtraSpace = lua_getextraspace(m_pState);
#else
    void* pExtraSpace = &g_pExtraSpace;
#endif
    void* pTemp = this;  // can't do &this, so we this temp variable
    memcpy(pExtraSpace, &pTemp, sizeof(void*));

    return true;
}

LuaVar LuaState::LoadString(const char* str)
{
    int result = luaL_loadbuffer(m_pState, str, strlen(str), str);
    if (result != LUA_OK)
    {
        const char* errorStr = lua_tostring(m_pState, -1);
        LUA_ERROR(errorStr);
        return LuaVar();
    }

    return LuaVar::CreateFromStack(this);
}

bool LuaState::DoString(const char* str) const
{
    LUA_ASSERT(m_pState);

    int error = luaL_dostring(m_pState, str);
    CHECK_FOR_LUA_ERROR(m_pState, error);

    return true;
}

bool LuaState::DoFile(const char* path)
{
    LUA_ASSERT(m_pState);

    StackHelpers::StackResetter resetter(m_pState, lua_gettop(m_pState));

    lua_pushcfunction(m_pState, &OnLuaException);       //  [exHandler]

    int result = luaL_loadfile(m_pState, path);         //  [exHandler, chunk|error]
    CHECK_FOR_PCALL_EXCEPTION(m_pState, result);

    result = lua_pcall(m_pState, 0, 0, -2);             //  [exHandler, error?]
    if (result != LUA_OK)
        return false;                                   //  []  <-- from StackResetter

    return true;                                        //  []  <-- from StackResetter
}

void LuaState::ClearStack() const
{
    lua_settop(m_pState, 0);
}

void LuaState::CollectGarbage() const
{
    lua_gc(m_pState, LUA_GCCOLLECT, 0);
}

LuaVar LuaState::GetGlobals()
{
    lua_pushglobaltable(m_pState);
    return LuaVar::CreateFromStack(this);
}

void LuaState::DumpStack(const char* prefix /*= nullptr*/) const
{
    DumpLuaStack(m_pState, prefix);
}

}  // end namespace BleachLua
