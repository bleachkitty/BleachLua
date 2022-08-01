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
#include "LuaVar.h"

//---------------------------------------------------------------------------------------------------------------------
// This iterator should work with a typical loop or a ranged-based for loop.  Examples:
// 
//=====================================
// Example of typical for loop:
// 
//      TableIterator endIt = table.end();
//      for (TableIterator it = table.begin(); it != endIt; ++it)
//      {
//          const TableIterator::KeyValuePair& keyValuePair = *it;
//          const LuaVar& key = keyValuePair.GetKey();
//          const LuaVar& value = keyValuePair.GetValue();
// 
//          // Assuming these the key is a string and the value is an int.  You would validate these, of course.
//          std::cout << "Key: " << keyValuePair.GetKey().GetString() << "; Val: " << keyValuePair.GetValue().GetInteger() << "\n";
//      }
// 
//=====================================
// Example of range-based for loop:
// 
//      for (auto& keyValuePair : table)
//      {
//          std::cout << "Key: " << keyValuePair.GetKey().GetString() << "; Val: " << keyValuePair.GetValue().GetInteger() << "\n";
//      }
// 
//=====================================
// Example of structured binding:
// 
//      for (auto [key, val] : table)
//      {
//          std::cout << "Key: " << key.GetString() << "; Val: " << val.GetInteger() << "\n";
//      }
//---------------------------------------------------------------------------------------------------------------------


namespace BleachLua {

class TableIterator
{
public:
    class KeyValuePair
    {
        friend class TableIterator;
        LuaVar m_key, m_value;

    public:
        KeyValuePair() = default;
        KeyValuePair(LuaVar&& key, LuaVar&& value) : m_key(std::move(key)), m_value(std::move(value)) { }
        KeyValuePair(const KeyValuePair&) = default;
        KeyValuePair(KeyValuePair&&) = default;
        KeyValuePair& operator=(const KeyValuePair&) = default;
        KeyValuePair& operator=(KeyValuePair&&) = default;
        ~KeyValuePair() = default;

        const LuaVar& GetKey() const { return m_key; }
        const LuaVar& GetValue() const { return m_value; }

        void Clear() { m_key.ClearRef(); m_value.ClearRef(); }

        // get() specialization for structure binding
        template <size_t N>
        decltype(auto) get() const
        {
            if constexpr (N == 0)
            {
                return m_key;
            }
            else if constexpr (N == 1)
            {
                return m_value;
            }
            else
            {
                // We used to do static_assert here, which worked, but is apparently ill-formed:
                // https://stackoverflow.com/questions/38304847/constexpr-if-and-static-assert
                // 
                // TODO C++ 20: Consider switching to the templated lambda solution in the above StackOverflow 
                // answer once C++ 20 is supported.  This will allow us to catch the error at compile-time rather 
                // than run-time.
                LUA_ERROR("N must be 0 or 1.");
                return LuaVar(m_key.GetLuaState());  // return nil
            }
        }
    };

private:
    LuaState* m_pState;
    KeyValuePair m_keyValue;
    bool m_isAtEnd;

public:
    // construction
    TableIterator() : m_pState(nullptr), m_isAtEnd(true) { }
    TableIterator(LuaState* pState, KeyValuePair&& keyValuePair) : m_pState(pState), m_keyValue(std::move(keyValuePair)), m_isAtEnd(false) { }
    TableIterator(const TableIterator&) = delete;
    TableIterator(TableIterator&& right) noexcept { Move(std::move(right)); }
    TableIterator& operator=(const TableIterator&) = delete;
    TableIterator& operator=(TableIterator&& right) noexcept { Move(std::move(right)); return (*this); }
    ~TableIterator();

    // Dereference operators.  Necessary for ranged-based for loops, though these are safe to call in other contexts.
    const KeyValuePair& operator*() const   { return m_keyValue; }
    KeyValuePair& operator*()               { return m_keyValue; }
    const KeyValuePair* operator->() const  { return &m_keyValue; }
    KeyValuePair* operator->()              { return &m_keyValue; }

    // Prefix increment.  Used for range-based for loops, though it's safe to call in other contexts.
    TableIterator& operator++();

    bool IsValid() const { return !m_isAtEnd; }

private:
    void SetToEnd();
    void Move(TableIterator&& right) noexcept;
};

// IMPORTANT!!!  This operator is used exclusively for range-base for loops to know when to end iteration.  It 
// will not return the expected result in any other case (the parameter is actually ignored).
inline bool operator!=(const TableIterator& left, [[maybe_unused]] const TableIterator& right)
{
    return left.IsValid();
}


}  // end namespace BleachLua


//---------------------------------------------------------------------------------------------------------------------
// Tuple template specializations, which are used to enable structure bindings on KeyValuePair.  Note that 
// specializing templates in the std namespace is a bit sketchy, but it works.  We have to use std instead of eastl 
// because structured bindings expect the std namespace.
//---------------------------------------------------------------------------------------------------------------------
namespace std
{
    template<>
    class tuple_size<BleachLua::TableIterator::KeyValuePair> : public integral_constant<size_t, 2> {};

    template <size_t N>
    class tuple_element<N, BleachLua::TableIterator::KeyValuePair>
    {
    public:
        using type = decltype(declval<BleachLua::TableIterator::KeyValuePair>().get<N>());
    };
}

