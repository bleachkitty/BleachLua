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

#include "LuaStl.h"

namespace BleachLua {

using LuaCFunction = int(*)(lua_State*);

// Bool
template <class Type> struct IsLuaBool { static constexpr bool value = false; };
template <> struct IsLuaBool<bool> { static constexpr bool value = true; };

// Integer
template <class Type> struct IsLuaInteger { static constexpr bool value = (luastl::is_integral<Type>::value && !IsLuaBool<Type>::value); };
template <class Type> struct IsLuaTrueInteger { static constexpr bool value = (luastl::is_integral<Type>::value && !luastl::is_same<Type, lua_Unsigned>::value && !IsLuaBool<Type>::value); };
template <class Type> struct IsLuaUnsigned { static constexpr bool value = luastl::is_same<Type, lua_Unsigned>::value; };

// Number
template <class Type> struct IsLuaNumber { static constexpr bool value = luastl::is_floating_point<Type>::value; };

// String
template <class Type> struct IsLuaString    { static constexpr bool value = false; };
template <> struct IsLuaString<const char*> { static constexpr bool value = true; };

// nil
template <class Type> struct IsLuaNil   { static constexpr bool value = false; };
template <> struct IsLuaNil<nullptr_t>  { static constexpr bool value = true; };

// function
template <class Type> struct IsLuaFunction      { static constexpr bool value = false; };
template <> struct IsLuaFunction<LuaCFunction>   { static constexpr bool value = true; };

// userdata
// Note: There is no way in C++ to determine whether something is userdata or lightuserdata purely by type.
template <class Type> struct IsLuaUserData  { static constexpr bool value = false; };
template <> struct IsLuaUserData<void*>     { static constexpr bool value = true; };

}  // end namespace BleachLua
