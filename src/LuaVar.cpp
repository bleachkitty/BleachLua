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

#include <BleachLua/LuaIncludes.h>
#include <BleachLua/LuaVar.h>
#include <BleachLua/LuaState.h>
#include <BleachLua/TableIterator.h>

#if BLEACHLUA_USE_MEMORY_POOLS
#include <BleachUtils/Memory/MemoryPool.h>
#endif

namespace BleachLua {

namespace _Internal {

#if BLEACHLUA_USE_MEMORY_POOLS
    BLEACH_MEMORYPOOL_DEFINITION(RefCount)
    BLEACH_MEMORYPOOL_AUTOINIT(RefCount, 1024 * 8)
#endif

}  // end namespace _Internal

LuaState* LuaVar::s_pDefaultLuaState = nullptr;

//---------------------------------------------------------------------------------------------------------------------
// Constructor / Destructor
//---------------------------------------------------------------------------------------------------------------------
LuaVar::LuaVar(LuaState* pState) noexcept
    : m_pState(pState)
    , m_reference(LUA_REFNIL)
    , m_pRefCount(nullptr)
{
    //
}

LuaVar::~LuaVar()
{
    ClearRef();
}

//---------------------------------------------------------------------------------------------------------------------
// Creates a LuaVar object from a variable at the top of the stack.  This pops the object from the stack.
//      -pState:    The lua state.
//      -return:    The constructed LuaVar object.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::CreateFromStack(LuaState* pState)
{
    LuaVar ret(pState);
    ret.CreateRegisteryEntryFromStack();
    return ret;
}

//---------------------------------------------------------------------------------------------------------------------
// Clears the Lua reference.  This effectively resets this variable, though the Lua state isn't cleared.  This will 
// cause the value to be garbage collected if there are no more references to it.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::ClearRef()
{
    if (m_reference != LUA_REFNIL)
    {
        // clean up the refcount
        LUA_ASSERT(m_pRefCount);  // this definitely shouldn't be null if we have a valid reference
        if (m_pRefCount->Decrement())
        {
            luaL_unref(m_pState->GetState(), LUA_REGISTRYINDEX, m_reference);
            BLEACHLUA_DELETE(m_pRefCount);
        }

        m_reference = LUA_REFNIL;
        m_pRefCount = nullptr;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// These set this variable to the appropriate value.
//      -val:   The val to set.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::SetString(const char* val)     { SetValue(val); }
void LuaVar::SetNil()                       { SetValue(nullptr); }
void LuaVar::SetBool(bool val)              { SetValue(val); }
void LuaVar::SetLightUserData(void* pVal)   { SetValue(pVal); }

//---------------------------------------------------------------------------------------------------------------------
// These get this variable's value as the appropriate type.  Note that if the variable isn't of the appropriate type, 
// it will return whatever the Lua runtime returns in those instances.
//      -retturn:   The value stored in this variable.
//---------------------------------------------------------------------------------------------------------------------
const char* LuaVar::GetString() const   { return GetValue<const char*>(); }
bool LuaVar::GetBool() const            { return GetValue<bool>(); }
void* LuaVar::GetLightUserData() const  { return GetValue<void*>(); }
void* LuaVar::GetUserData() const       { return GetValue<void*>(); }

//---------------------------------------------------------------------------------------------------------------------
// Returns true if this variable is of the given type.
//      -return:    true if this variable is of the given type, false if not.
//---------------------------------------------------------------------------------------------------------------------
bool LuaVar::IsInteger() const          { return IsType<lua_Integer>(); }
bool LuaVar::IsNumber() const           { return IsType<lua_Number>(); }
bool LuaVar::IsString() const           { return IsType<const char*>(); }
bool LuaVar::IsNil() const              { return (m_reference == LUA_REFNIL); }
bool LuaVar::IsBool() const             { return IsType<bool>(); }
bool LuaVar::IsLightUserData() const    { return IsType<void*>(); }
bool LuaVar::IsUserData() const         { return IsType<void*>(); }
bool LuaVar::IsFunction() const         { return DoLuaAction([this]() -> bool { return lua_isfunction(m_pState->GetState(), -1); }); }
bool LuaVar::IsCFunction() const        { return DoLuaAction([this]() -> bool { return lua_iscfunction(m_pState->GetState(), -1); }); }
bool LuaVar::IsTable() const            { return DoLuaAction([this]() -> bool { return lua_istable(m_pState->GetState(), -1); }); }

//---------------------------------------------------------------------------------------------------------------------
// Returns the string type name for this variable.
//      -return:    The string type name for this variable.
//---------------------------------------------------------------------------------------------------------------------
const char* LuaVar::GetTypeName() const
{
    return DoLuaAction([this]() -> const char*
    {
        int type = lua_type(m_pState->GetState(), -1);
        return lua_typename(m_pState->GetState(), type);
    }, true);  // true to allow nil values
}

luastl::string LuaVar::GetTypeNameStr() const
{
    luastl::string result = GetTypeName();
    return result;
}

//---------------------------------------------------------------------------------------------------------------------
// Sets a field in this table to the given value.
// IMPORTANT: This variable must be a table.
//      -key:   The key of the field.
//      -val:   The value to set.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::SetTableVar(const char* key, const LuaVar& val) const      { SetTableValue(key, val); }
void LuaVar::SetTableVar(const char* key, LuaVar&& val) const           { SetTableValue(key, std::move(val)); }
void LuaVar::SetTableString(const char* key, const char* val) const     { SetTableValue(key, val); }
void LuaVar::SetTableNil(const char* key) const                         { SetTableValue(key, nullptr); }
void LuaVar::SetTableBool(const char* key, bool val) const              { SetTableValue(key, val); }
void LuaVar::SetTableLightUserData(const char* key, void* pVal) const { SetTableValue(key, pVal); }

//---------------------------------------------------------------------------------------------------------------------
// Sets a field on this table to a newly created table.
// IMPORTANT: This variable must be a table.
//      -key:               The key of the field.
//      -nativeArraySize:   The size of the array portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -hashSize:          The size of the hash map portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -return:            The newly created table.  This will already have been added to this variable's field.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::SetNewTable(const char* key, int nativeArraySize, int hashSize) const
{
    LUA_ASSERT(m_pState);
    LuaVar table(m_pState);
    table.CreateNewTable(nativeArraySize, hashSize);
    SetTableValue(key, table);
    return table;
}

//---------------------------------------------------------------------------------------------------------------------
// Sets a field on this table to a newly created table.  This version of the function exists to suppress C26444, which 
// is one of the Herb Sutter / Bjarne Stroustrup quality rules: 
// https://docs.microsoft.com/en-us/visualstudio/code-quality/c26444?view=vs-2019
// 
// IMPORTANT: This variable must be a table.
//      -key:               The key of the field.
//      -nativeArraySize:   The size of the array portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -hashSize:          The size of the hash map portion of the table, or 0 to use Lua's default.  Defaults to 0.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::SetNewTableNoReturn(const char* key, int nativeArraySize, int hashSize) const
{
    auto newTable = SetNewTable(key, nativeArraySize, hashSize);
}

//---------------------------------------------------------------------------------------------------------------------
// Creates a new table and pushes it to the end of the array section of this table.
// IMPORTANT: This variable must be a table.
//      -nativeArraySize:   The size of the array portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -hashSize:          The size of the hash map portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -return:            The newly created table.  This will already have been added to the table.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::InsertNewTableAtEnd(int nativeArraySize, int hashSize) const
{
    LUA_ASSERT(m_pState);
    LuaVar table(m_pState);
    table.CreateNewTable(nativeArraySize, hashSize);
    Insert(table);
    return table;
}

//---------------------------------------------------------------------------------------------------------------------
// Creates a new table and pushes it to the end of the array section of this table.  This version of the function 
// exists to suppress C26444, which is one of the Herb Sutter / Bjarne Stroustrup quality rules: 
// https://docs.microsoft.com/en-us/visualstudio/code-quality/c26444?view=vs-2019
// 
// IMPORTANT: This variable must be a table.
//      -nativeArraySize:   The size of the array portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -hashSize:          The size of the hash map portion of the table, or 0 to use Lua's default.  Defaults to 0.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::InserNewTableAtEndNoReturn(int nativeArraySize, int hashSize) const
{
    auto newTable = InsertNewTableAtEnd(nativeArraySize, hashSize);
}

//---------------------------------------------------------------------------------------------------------------------
// Gets a field in this table to the given value.
// IMPORTANT: This variable must be a table.
//      -key:       The key of the field.
//      -return:    The value at that field.  Note that if the variable isn't of the appropriate type, it will return 
//                  whatever the Lua runtime returns in those instances.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::GetTableVar(const char* key) const           { return GetTableValue<LuaVar>(key); }
const char* LuaVar::GetTableString(const char* key) const   { return GetTableValue<const char*>(key); }
bool LuaVar::GetTableBool(const char* key) const            { return GetTableValue<bool>(key); }
void* LuaVar::GetTableLightUserData(const char* key) const  { return GetTableValue<void*>(key); }
void* LuaVar::GetTableUserData(const char* key) const       { return GetTableValue<void*>(key); }

//---------------------------------------------------------------------------------------------------------------------
// Gets a table from this table, or creates it if it doesn't exist.
//      -key:   //
//      -nativeArraySize:   If the table needs to be created, this is the size of the array portion of the table, or 
//                          0 to use Lua's default.  This parameter is ignored if the table already exists.  Defaults 
//                          to 0.
//      -hashSize:          If the table needs to be created, this is the size of the hash map portion of the table, 
//                          or 0 to use Lua's default.  This parameter is ignored if the table already exists.
//                          Defaults to 0.
//      -return:            The table that was found or created, or nil if something went wrong.  This typically 
//                          means that a non-table element was already at this key.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::GetOrCreateNewTable(const char* key, int nativeArraySize /*= 0*/, int hashSize /*= 0*/) const
{
    LuaVar table = GetTableVar(key);

    // found a table, so return it
    if (table.IsTable())
        return table;

    // nothing is at they key, so modify it now
    if (table.IsNil())
        return SetNewTable(key, nativeArraySize, hashSize);

    // if we get here, something was in the way
    LUA_ERROR("Found something at key " + luastl::string(key) + " but it wasn't a table.  Type is " + table.GetTypeNameStr());
    return LuaVar();
}

//---------------------------------------------------------------------------------------------------------------------
// Instantiates a new table and stores it in this variable.
//      -nativeArraySize:   The size of the array portion of the table, or 0 to use Lua's default.  Defaults to 0.
//      -hashSize:          The size of the hash map portion of the table, or 0 to use Lua's default.  Defaults to 0.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::CreateNewTable(int nativeArraySize /*= 0*/, int hashSize /*= 0*/)
{
    LUA_ASSERT(m_pState);
    ClearRef();
                                                                            // Stack:
    lua_createtable(m_pState->GetState(), nativeArraySize, hashSize);       //  [t]
    CreateRegisteryEntryFromStack();                                        //  []
}

//---------------------------------------------------------------------------------------------------------------------
// Table iteration functions.  Note that these don't follow our naming convention so that we can take advantage of 
// range-based for loops.  Note that end() is just a dummy value.
//      -return:    Returns the iterator for this table.
//---------------------------------------------------------------------------------------------------------------------
TableIterator LuaVar::begin()       { return InternalBegin(); }
TableIterator LuaVar::end()         { return InternalEnd(); }
TableIterator LuaVar::begin() const { return InternalBegin(); }
TableIterator LuaVar::end() const   { return InternalEnd(); }

//---------------------------------------------------------------------------------------------------------------------
// Looks up a table.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::Lookup(const luastl::string& path) const
{
    // not a table
    if (!IsTable())
    {
        LUA_ERROR("Attempting to call Lookup() on var that isn't a table.  Type is " + GetTypeNameStr());
        return LuaVar();
    }

    // split the path
    luastl::vector<luastl::string> splitPath;
    size_t start = 0;
    size_t end = path.find_first_of('.', 0);
    while (end != luastl::string::npos)
    {
        splitPath.emplace_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find_first_of('.', start);
    }

    // Add the final chunk of the string.  Note that this check is required because if path is empty, an empty 
    // string would end up getting pushed.
    if (luastl::string finalPiece = path.substr(start, end - start); !finalPiece.empty())
        splitPath.emplace_back(std::move(finalPiece));

    // empty path
    if (splitPath.empty())
    {
        LUA_ERROR("Empty path in Lookup(): " + path);
        return LuaVar();
    }

    // only one item
    if (splitPath.size() == 1)
        return GetTableVar(splitPath[0].c_str());

    // perform the lookup
    LuaVar curr = (*this);
    for (size_t i = 0; i < splitPath.size() - 1; ++i)
    {
        curr = curr.GetTableVar(splitPath[i].c_str());
        if (!curr.IsTable())
        {
            // Note: If you get here after calling Tuning::GetXXX(), and it's complaining that Tuning is nil, you 
            // likely passed in something like "Tuning.Foo.bar".  The tuning functions already add "Tuning" to the 
            // front, so you should pass in "Foo.bar" instead.  I don't want to change the error message because 
            // this function is super-generic, but it's a common enough problem that it warrants a comment.
            LUA_ERROR("Attempting to call Lookup() when one of the elements isn't a table.  Full path is " + path + " and element is " + splitPath[i] + ".  Type is " + curr.GetTypeNameStr());
            return LuaVar();
        }
    }

    // we're now at the last var, so return the final variable
    return curr.GetTableVar(splitPath.back().c_str());
}

//---------------------------------------------------------------------------------------------------------------------
// Gets the length of the objects.  For strings, this is the string length.  For tables, this is the result of the 
// length operator ('#') with no metamethods, so it only counts the array portion, not the hashtable portion.  For 
// userdata, this is the size of the block of memory allocated for the userdata.  Every other value returns 0.
//      -return:    The length of the variable.
//---------------------------------------------------------------------------------------------------------------------
size_t LuaVar::GetLength() const
{
    return DoLuaAction([this]() -> size_t { return lua_rawlen(m_pState->GetState(), -1); });
}

//---------------------------------------------------------------------------------------------------------------------
// Gets the number of elements in this table.  Unlike GetLength(), this includes the hash portion and the array 
// portion of the table.  The trade-off is that this is O(n) since we have to loop through the entire table while 
// GetLength() is (presumebly) O(1).
//      -return:    The number of elements in the array.
//---------------------------------------------------------------------------------------------------------------------
size_t LuaVar::GetNumElements() const
{
    if (!IsTable())
    {
        LUA_ERROR("Trying to get the number of elements of a non-table.  Type is " + GetTypeNameStr());
        return 0;
    }

    size_t count = 0;
    for ([[maybe_unused]] auto& element : (*this))
    {
        ++count;
    }

    return count;
}

//---------------------------------------------------------------------------------------------------------------------
// Sets the meta table for this variable.  This variable must be a table.
//      -metaTable: The meta table to set.  This must be a table.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::SetMetaTable(const LuaVar& metaTable) const
{
    LUA_ASSERT(IsTable() || IsUserData());  // technically valid for other types, but likely not what we want
    LUA_ASSERT(metaTable.IsTable());
    LUA_ASSERT(m_pState == metaTable.m_pState);

    PushValueToStack(false);                        //  [var]
    metaTable.PushValueToStack();                   //  [var, mt]
    lua_setmetatable(m_pState->GetState(), -2);     //  [var]
    lua_pop(m_pState->GetState(), 1);               //  []
}

//---------------------------------------------------------------------------------------------------------------------
// Gets the metatable for this variable.  It must be a table.
//      -return:    The metatable, or nil if there is no metatable.
//---------------------------------------------------------------------------------------------------------------------
LuaVar LuaVar::GetMetaTable() const
{
    LUA_ASSERT(IsTable());
    return DoLuaAction([this]()
    {
        LuaVar ret;                                                         //  [var]
        const int success = lua_getmetatable(m_pState->GetState(), -1);     //  [var, mt?]
        if (success)
            ret = CreateFromStack(m_pState);                                //  [var]
        return ret;
    });
}

//---------------------------------------------------------------------------------------------------------------------
// This function wraps the given pointer in a userdata object.  The purpose of this is to wrap "this" pointers of 
// C++ objects and place a metatable on them to allow Lua access to the C++ side.  The extra memory allocation is not 
// ideal, but these need to be userdata rather than lightuserdata since only tables and userdata can have unique 
// metatables.
//      -pPtr:  The pointer to wrap.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::WrapObjectPtr(void* pPtr)
{
    LUA_ASSERT(m_pState);
    LUA_ASSERT(pPtr);

    // clear whatever the var was holding
    ClearRef();

    // allocate a new userdatum and copy over the pointer
    void* pUserData = lua_newuserdata(m_pState->GetState(), sizeof(void*));     //  [userdata]
    memcpy(pUserData, &pPtr, sizeof(void*));

    // create the Lua object
    CreateRegisteryEntryFromStack();                                            //  []
}

//---------------------------------------------------------------------------------------------------------------------
// Pushes the referenced variable to the top of the Lua stack.
//      -allowNil:  If true and this value is nil, nil will be pushed to the top of the stack.  If false, we don't 
//                  push anything and return false.  Defaults to true.
//      -return:    true if the value was pushed, false if not.
//---------------------------------------------------------------------------------------------------------------------
bool LuaVar::PushValueToStack(bool allowNil /*= true*/) const
{
    if (IsValid())
        lua_rawgeti(m_pState->GetState(), LUA_REGISTRYINDEX, m_reference);
    else if (allowNil)
        lua_pushnil(m_pState->GetState());
    else
        return false;
    return true;
}

//---------------------------------------------------------------------------------------------------------------------
// Creates the registry entry for this var.  Assumes that it is not currently holding onto a reference.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::CreateRegisteryEntryFromStack()
{
    LUA_ASSERT(!m_pRefCount);
    LUA_ASSERT(m_reference == LUA_REFNIL);
    m_reference = luaL_ref(m_pState->GetState(), LUA_REGISTRYINDEX);  // adds the top of the stack as a new value in the registry, returning the reference to it
    if (m_reference != LUA_REFNIL)
        m_pRefCount = BLEACHLUA_NEW(_Internal::RefCount);
}

//---------------------------------------------------------------------------------------------------------------------
// Helper function that does the actual comparison.  This uses lua_compare(), which will compare the variables as if 
// you call == in Lua, so it will respect metamethods and so on.
//      -right:     The right side of the equation.
//      -op:        The operator to call, which is one of the LUA_OP* values.
//      -return:    true if the comparison is true, false if not.
//---------------------------------------------------------------------------------------------------------------------
bool LuaVar::CompareHelper(const LuaVar& right, int op) const
{
    LUA_ASSERT(m_pState);

    // push the values onto the stack
    PushValueToStack();                                             //  [left]
    right.PushValueToStack();                                       //  [left, right]

    // compare the values and clean up the stack
    int result = lua_compare(m_pState->GetState(), -1, -2, op);     //  [left, right]
    lua_pop(m_pState->GetState(), 2);                               //  []

    return result == 1;
}

//---------------------------------------------------------------------------------------------------------------------
// Copy and move functionality, used by the copy/move constructors & assignment operators.
//---------------------------------------------------------------------------------------------------------------------
void LuaVar::Copy(const LuaVar& right)
{
    ClearRef();

    m_pState = right.m_pState;
    m_reference = right.m_reference;
    m_pRefCount = right.m_pRefCount;
    if (m_pRefCount)
        m_pRefCount->Increment();
}

void LuaVar::Move(LuaVar&& right) noexcept
{
    ClearRef();

    m_pState = right.m_pState;
    m_reference = right.m_reference;
    m_pRefCount = right.m_pRefCount;

    right.m_pState = nullptr;
    right.m_reference = LUA_REFNIL;
    right.m_pRefCount = nullptr;
}

//---------------------------------------------------------------------------------------------------------------------
// Internal workhorse functions begin() and end().  This is here because the const and non-const versions of begin() 
// and end() are the same internally.
//      -return:    Returns the iterator for this table.
//---------------------------------------------------------------------------------------------------------------------
TableIterator LuaVar::InternalBegin() const
{
    // make sure this variable is valid
    if (!IsValid())
    {
        LUA_ERROR("Trying to get an iterator for an invalid variable.");
        return TableIterator();
    }

    // push the table onto the stack
    PushValueToStack();                                                                             //  [t]
    if (!lua_istable(m_pState->GetState(), -1))
    {
        LUA_ERROR("Trying to get an iterator for a variable that isn't a table.");
        lua_pop(m_pState->GetState(), 1);  // need to pop the variable off the stack                //  []
        return TableIterator();
    }

    // If we get here, we have a valid table and it's on the stack.  Next step is to push nil, which represents the 
    // first key.
    lua_pushnil(m_pState->GetState());                                                              //  [t, nil]

    // Call lua_next() to go to the first valid element.
    const int result = lua_next(m_pState->GetState(), -2);  // table is at -2                       //  [t, key?, val?]

    // if lua_next() returns 0 here, it means that the table is empty.
    if (result == 0)
    {
        lua_pop(m_pState->GetState(), 1);                                                           //  []  <-- only called if the above weren't pushed
        return TableIterator();                                                                     
    }

    // We need to copy the key since CreateFromStack() will pop it from the stack.  Internally, it calls luaL_ref(), 
    // so we have no control over this.  The stack should contain: [key, val, key, table]
    lua_pushvalue(m_pState->GetState(), -2);                                                        //  [t, key, val, key]  <-- lua_next() succeeded here

    // If we get here, we have a valid entry.  Build up the TableIterator object.
    LuaVar key = CreateFromStack(m_pState);                                                         //  [t, key, val]
    LuaVar value = CreateFromStack(m_pState);                                                       //  [t, key]
    TableIterator iterator(m_pState, TableIterator::KeyValuePair(std::move(key), std::move(value)));

    // Note: when this function returns, the stack should contain [key, table].  This is necessary so that it 
    // corretly goes to the next element.
    return iterator;                                                                                //  [t, key]  <-- necessary to continue iteration
}

TableIterator LuaVar::InternalEnd() const
{
    return TableIterator();
}

}  // end namespace BleachLua

