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

#include <BleachLua/TableIterator.h>
#include <BleachLua/LuaState.h>

namespace BleachLua {

TableIterator::~TableIterator()
{
    // This state can occur if you break out of an iteration early.  We still need to clean up the stack.  The top 
    // of operator++() is always assumed to have two elements: the table and the previous key.  We can safely assume 
    // that we need to pop two elements from the stack to reset it.  This is kind of dangerous since we don't truly 
    // know the state of the iterator.  Alternatively, we could use a member StackResetter, but I think that might 
    // cause sync issues.
    if (!m_isAtEnd)
        lua_pop(m_pState->GetState(), 2);
}

TableIterator& TableIterator::operator++()
{
    LUA_ASSERT(m_pState);                                           // Stack:
                                                                    //  [t, key]  <-- This comes from the call to LuaVar::begin() or previous calls to this function
    // Try to move to the next element.  If it's invalid, we're done iterating.
    if (lua_next(m_pState->GetState(), -2) == 0)                    //  [t, key?, val?]  <-- note that key will be the next key, not the old key
    {
        SetToEnd();                                                 //  [t]  <-- failed, so no key or val; SetToEnd() will clean up the stack
    }

    // Next element is valid, so set the key/value pair
    else
    {
        // We need to copy the key since CreateFromStack() will pop it from the stack.  Internally, it calls luaL_ref(), 
        // so we have no control over this.  The stack should contain: [key, val, key, table]
        lua_pushvalue(m_pState->GetState(), -2);                    //  [t, key, val, key]  <-- success case, so key and val are present

        m_keyValue.m_key = LuaVar::CreateFromStack(m_pState);       //  [t, key, val]
        m_keyValue.m_value = LuaVar::CreateFromStack(m_pState);     //  [t, key]
    }

    return (*this);                                                 //  [t, key]  <-- this is expected so it can seed the next call to lua_next()
}

void TableIterator::SetToEnd()
{
    m_isAtEnd = true;
    m_keyValue.Clear();

    // we're assumed to just have the table on the stack at this point since lua_next() failed
    lua_pop(m_pState->GetState(), 1);
}

void TableIterator::Move(TableIterator&& right) noexcept
{
    m_pState = right.m_pState;
    m_keyValue = std::move(right.m_keyValue);
    m_isAtEnd = right.m_isAtEnd;

    right.m_pState = nullptr;
    right.m_isAtEnd = true;  // prevents destructor from resetting the stack
}

}  // end namespace BleachLua
