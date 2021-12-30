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
#include "LuaConfig.h"
#include "LuaTypeTraits.h"
#include "InternalLuaState.h"
#include "LuaStl.h"
#include "LuaError.h"
#include <cmath>

namespace BleachLua {

class LuaState;

namespace StackHelpers {

//---------------------------------------------------------------------------------------------------------------------
// Important note about unsigned 64-bit ints:
// 
// Lua doesn't support unsigned 64-bit ints, only signed 64-bit ints.  Anything that is larger than 0x7fff'ffff'ffff'ffff 
// will be treated as a number (i.e. a double).  This won't work because if we have values large enough to require an 
// unsigned 64-bit int, we'll end up running into floating-point preceision errors, even with doubles.
// 
// We try to make it work as best we can by casting back and forth to/from signed/unsigned values.  This isn't ideal, 
// but the only other alternatives would be either to fail outright, or to cast to a LuaNumber.  Failing isn't a good 
// option, and casting to a LuaNumber won't work because of the precision issues mentioned above.
//---------------------------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------------------------
// GetDefault()
//---------------------------------------------------------------------------------------------------------------------
// bool
template <class Type>
luastl::enable_if_t<IsLuaBool<Type>::value, Type> GetDefault()
{
    return false;
}

// integer
template <class Type>
luastl::enable_if_t<IsLuaInteger<Type>::value, Type> GetDefault()
{
    return 0;
}

// number
template <class Type>
luastl::enable_if_t<IsLuaNumber<Type>::value, Type> GetDefault()
{
    return 0;
}

// string 
template <class Type>
luastl::enable_if_t<IsLuaString<Type>::value, Type> GetDefault()
{
    return nullptr;
}

// nil
template <class Type>
luastl::enable_if_t<IsLuaNil<Type>::value, Type> GetDefault()
{
    return nullptr;
}

// function
template <class Type>
luastl::enable_if_t<IsLuaFunction<Type>::value, Type> GetDefault()
{
    return nullptr;
}

// userdata
// Note: We get as userdata
template <class Type>
luastl::enable_if_t<IsLuaUserData<Type>::value, Type> GetDefault()
{
    return nullptr;
}

// Note: The LuaVar version of GetDefault() is in LuaVar.h

//---------------------------------------------------------------------------------------------------------------------
// Push()
//---------------------------------------------------------------------------------------------------------------------
// bool
template <class Type>
luastl::enable_if_t<IsLuaBool<Type>::value> Push(LuaState* pState, Type val)
{
    LUA_ASSERT(pState);
    lua_pushboolean(pState->GetState(), val);
}

// integer
template <class Type>
luastl::enable_if_t<IsLuaInteger<Type>::value> Push(LuaState* pState, Type val)
{
    LUA_ASSERT(pState);

    // Note that we're casting to a signed value, so uint64_t values that are too large will end up being negative.
    // Unsigned 32-bit values won't be changed because static_cast tries to maintain the value of the number if it 
    // can.
    lua_pushinteger(pState->GetState(), static_cast<lua_Integer>(val));
}

// number
template <class Type>
luastl::enable_if_t<IsLuaNumber<Type>::value> Push(LuaState* pState, Type val)
{
    LUA_ASSERT(pState);
    lua_pushnumber(pState->GetState(), static_cast<lua_Number>(val));
}

// string 
template <class Type>
luastl::enable_if_t<IsLuaString<Type>::value> Push(LuaState* pState, Type val)
{
    LUA_ASSERT(pState);
    lua_pushstring(pState->GetState(), val);
}

// nil
template <class Type>
luastl::enable_if_t<IsLuaNil<Type>::value> Push(LuaState* pState, [[maybe_unused]] Type val)
{
    LUA_ASSERT(pState);
    lua_pushnil(pState->GetState());
}

// function
template <class Type>
luastl::enable_if_t<IsLuaFunction<Type>::value> Push(LuaState* pState, Type val)
{
    LUA_ASSERT(pState);
    lua_pushcfunction(pState->GetState(), val);
}

// userdata
// Note: We push data as lightuserdata.
template <class Type>
luastl::enable_if_t<IsLuaUserData<Type>::value> Push(LuaState* pState, Type val)
{
    LUA_ASSERT(pState);
    lua_pushlightuserdata(pState->GetState(), val);
}

// Note: The LuaVar version of Push() is in LuaVar.h

//---------------------------------------------------------------------------------------------------------------------
// Is()
//---------------------------------------------------------------------------------------------------------------------
// bool
template <class Type>
luastl::enable_if_t<IsLuaBool<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_isboolean(pState->GetState(), stackIndex);
}

// integer
#if BLEACHLUA_CORE_VERSION >= 53

// Typical case for int
template <class Type>
luastl::enable_if_t<IsLuaTrueInteger<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_isinteger(pState->GetState(), stackIndex);
}

// Special case for uint64_t, which Lua doesn't support.  This is exactly the same as the case above except that 
// in debug mode we kick out an error if we detect a truly unsigned 64-bit int.  See the comment at the top of 
// this file for details.
template <class Type>
luastl::enable_if_t<IsLuaUnsigned<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    if (lua_isinteger(pState->GetState(), stackIndex))
    {
        return true;
    }
#if BLEACHLUA_DEBUG_MODE
    else if (lua_isnumber(pState->GetState(), stackIndex))
    {
        const lua_Number num = lua_tonumber(pState->GetState(), stackIndex);
        if (num - std::floor(num) == 0.0)
        {
            LUA_ERROR("You've passed in a value to Is() that's greater than the max size allowed by a signed 64-bit int.  Lua doesn't support unsigned 64-bit ints, so you must convert this to a signed int first.  It will be converted back to an unsigned int when passed into C++.");
        }
    }
#endif

    return false;
}

#else  // BLEACHLUA_CORE_VERSION <= 52

template <class Type>
luastl::enable_if_t<IsLuaTrueInteger<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);

    // TODO: Fix the Window system so we can revert this.
#if 1
    // Lua 5.2 and below have no internal concept of an int, so we have to get the number and subtract the floor of 
    // it.  If it's non-zero, this can't be an int.
    if (lua_isnumber(pState->GetState(), stackIndex))
    {
        const lua_Number num = lua_tonumber(pState->GetState(), stackIndex);
        if (num - std::floor(num) == 0)
            return true;
    }

    return false;
#else
    return lua_isnumber(pState->GetState(), stackIndex);
#endif
}
#endif  // BLEACHLUA_CORE_VERSION

// number
template <class Type>
luastl::enable_if_t<IsLuaNumber<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_isnumber(pState->GetState(), stackIndex);
}

// string
template <class Type>
luastl::enable_if_t<IsLuaString<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_isstring(pState->GetState(), stackIndex);
}

// nil
template <class Type>
luastl::enable_if_t<IsLuaNil<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_isnil(pState->GetState(), stackIndex);
}

// function
template <class Type>
luastl::enable_if_t<IsLuaFunction<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_iscfunction(pState->GetState(), stackIndex);
}

// userdata
// Note: Either userdata or lightuserdata will be valid.
template <class Type>
luastl::enable_if_t<IsLuaUserData<Type>::value, bool> Is(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return (lua_islightuserdata(pState->GetState(), stackIndex) || lua_isuserdata(pState->GetState(), stackIndex));
}

// Note: The LuaVar version of Is() is in LuaVar.h

//---------------------------------------------------------------------------------------------------------------------
// Get()
//---------------------------------------------------------------------------------------------------------------------
// bool
template <class Type>
luastl::enable_if_t<IsLuaBool<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_toboolean(pState->GetState(), stackIndex);
}

// integer
#if BLEACHLUA_CORE_VERSION >= 53

// We need two versions of Get() for integers, one for unsigned 64-bit ints and one for everything else.  This is 
// the path for all ints that aren't uint64_t's.
template <class Type>
luastl::enable_if_t<IsLuaTrueInteger<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    int success = 0;
    const Type ret = static_cast<Type>(lua_tointegerx(pState->GetState(), stackIndex, &success));
    if (!success)
    {
        LUA_ERROR("Failed to convert value to integer.  Type is " + luastl::string(lua_typename(pState->GetState(), lua_type(pState->GetState(), -1))));
        return GetDefault<Type>();
    }
    return ret;
}

// This is the special-case for uint64_t.  If the value originally came from C++, it should have been converted into 
// a signed 64-bit int, so we should able to read it normally and cast it to unsigned here.  The purpose of this 
// function is to have some additional error checking in case an unsigned 64-bit literal (or something similar) is 
// passed in.
// 
// Important: This function is exactly the same as the one above except for the extra error checking.  When/if we 
// decide to promote BleachLua to C++ 17, this really needs to be an if constexpr.
template <class Type>
luastl::enable_if_t<IsLuaUnsigned<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    int success = 0;
    const Type ret = static_cast<Type>(lua_tointegerx(pState->GetState(), stackIndex, &success));
    if (!success)
    {
#if BLEACHLUA_DEBUG_MODE
        if (Is<lua_Number>(pState, stackIndex))
        {
            const lua_Number num = Get<lua_Number>(pState, stackIndex);
            if (num - std::floor(num) == 0)
            {
                LUA_ERROR("You've passed in a value to Get() that's greater than the max size allowed by a signed 64-bit int.  Lua doesn't support unsigned 64-bit ints, so you must convert this to a signed int first.  It will be converted back to an unsigned int when passed into C++.");
            }
        }
        else
        {
            LUA_ERROR("Failed to convert value to uint64_t.  Type is " + luastl::string(lua_typename(pState->GetState(), lua_type(pState->GetState(), -1))));
        }
#endif  // BLEACHLUA_DEBUG_MODE
        return GetDefault<Type>();
    }
    return ret;
}

#else  // BLEACHLUA_CORE_VERSION < 53

template <class Type>
luastl::enable_if_t<IsLuaInteger<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    // In Lua 5.2, lua_tointegerx() will return 0x80000000 if you try to convert a number that's larger than 
    // 0x7fffffff, so converting something like 4'000'000'000 to an unsigned int won't work, even though the 
    // unsigned int can hold that value.  The lame workaround is to grab the number and do the cast with a 
    // static_cast.
    int success = 0;
    lua_Number tempNum = lua_tonumberx(pState->GetState(), stackIndex, &success);
    Type ret = static_cast<Type>(tempNum);
    if (!success)
        LUA_ERROR("Failed to convert value to integer.  Type is " + luastl::string(lua_typename(pState->GetState(), lua_type(pState->GetState(), -1))));
    return ret;
}
#endif

// number
template <class Type>
luastl::enable_if_t<IsLuaNumber<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return static_cast<Type>(lua_tonumber(pState->GetState(), stackIndex));
}

// string 
template <class Type>
luastl::enable_if_t<IsLuaString<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_tostring(pState->GetState(), stackIndex);
}

// nil
template <class Type>
luastl::enable_if_t<IsLuaNil<Type>::value, Type> Get([[maybe_unused]] LuaState* pState, [[maybe_unused]] int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return nullptr;
}

// function
template <class Type>
luastl::enable_if_t<IsLuaFunction<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_tocfunction(pState->GetState(), stackIndex);
}

// userdata
// Note: We get as userdata (as opposed to lightuserdata)
template <class Type>
luastl::enable_if_t<IsLuaUserData<Type>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    LUA_ASSERT(pState);
    return lua_touserdata(pState->GetState(), stackIndex);
}

// Note: The LuaVar version of Get() is in LuaVar.h

//---------------------------------------------------------------------------------------------------------------------
// Misc
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
Type GetFromStack(LuaState* pState, [[maybe_unused]] bool showErrorOnUnderrun = true)
{
    LUA_ASSERT(pState);

    if (lua_gettop(pState->GetState()) > 0)
    {
        Type value = Get<Type>(pState);
        lua_pop(pState->GetState(), 1);  // make sure we pop since we have no idea how many parameters there are
        return value;
    }
    else
    {
#if BLEACHLUA_DEBUG_MODE
        if (showErrorOnUnderrun)
            LUA_ERROR("Lua stack underrun: Trying to pop value from empty stack.");
#endif
        return GetDefault<Type>();
    }
}

class StackResetter
{
    lua_State* m_pState;
    int m_oldTop;

public:
    explicit StackResetter(lua_State* pState, int oldTop = 0) : m_pState(pState), m_oldTop(oldTop) { }
    ~StackResetter() { lua_settop(m_pState, m_oldTop); }
    StackResetter& operator++() { ++m_oldTop; return (*this); }  // increments where the stack is reset to; used for functions that need to retrun something to Lua via the stack
};

}  // end namespace StackHelpers

}  // end namespace BleachLua

