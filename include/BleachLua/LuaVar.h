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
#include "LuaTypes.h"
#include "StackHelpers.h"
#include "LuaError.h"
#include "LuaDebug.h"
#include "LuaStl.h"

#if BLEACHLUA_USE_MEMORY_POOLS
#include <BleachUtils/Memory/MemoryMacros.h>
#endif

namespace BleachLua {

namespace _Internal {

//---------------------------------------------------------------------------------------------------------------------
// RefCount
// 
// The LuaVar ref count.  This is used to allow multiple LuaVar's to point to the same underlying object.
//---------------------------------------------------------------------------------------------------------------------
class RefCount
{
#if BLEACHLUA_USE_MEMORY_POOLS
    BLEACH_MEMORYPOOL_DECLARATION(0)
#endif

    size_t m_refCount;

public:
    RefCount() : m_refCount(1) { }  // we're spinning up a new refcount, so start it at one
    RefCount(const RefCount&) = delete;
    RefCount& operator=(const RefCount&) = delete;
    RefCount(RefCount&&) = delete;
    RefCount& operator=(RefCount&&) = delete;

    void Increment() { ++m_refCount; }
    bool Decrement()
    {
#if BLEACHLUA_DEBUG_MODE
        if (m_refCount > 0)
            --m_refCount;
        else
            LUA_ERROR("Attempting to decrement ref count, but it's already 0.");
#else
        --m_refCount;
#endif

        return (m_refCount == 0);
    }

    bool IsValid() const { return m_refCount > 0; }
};

}  // end namespace BleachLua::_Internal


class LuaState;
class TableIterator;

//---------------------------------------------------------------------------------------------------------------------
// LuaVar
// 
// This class represents a single Lua variable.  The resource is stored in the Lua registry, so a reference is 
// maintained as long as this variable exists, even if all references in Lua code are garbage collected.
//---------------------------------------------------------------------------------------------------------------------
class LuaVar
{
    friend class LuaState;
    friend class TableIterator;

public:
    using NativeInt = lua_Integer;
    using NativeNumber = lua_Number;

private:
    static LuaState* s_pDefaultLuaState;

    LuaState* m_pState;
    _Internal::RefCount* m_pRefCount;
    int m_reference;

public:
    static void SetDefaultLuaState(LuaState* pLuaState) { s_pDefaultLuaState = pLuaState; }

    LuaVar() noexcept : m_pState(s_pDefaultLuaState), m_reference(LUA_REFNIL), m_pRefCount(nullptr) { }
    explicit LuaVar(LuaState* pState) noexcept;
    LuaVar(const LuaVar& right) : LuaVar()      { Copy(right); }
    LuaVar(LuaVar&& right) noexcept : LuaVar()  { Move(std::move(right)); }
    LuaVar& operator=(const LuaVar& right)
    {
        if (&right == this)
            return *this;
        Copy(right);
        return (*this);
    }
    LuaVar& operator=(LuaVar&& right) noexcept 
    {
        if (&right == this)
            return *this;
        Move(std::move(right));
        return (*this);
    }
    ~LuaVar();

    static LuaVar CreateFromStack(LuaState* pState);

    void ClearRef();  // effectively destroys this object, though m_pState will not be changed
    bool IsValid() const { return (m_pState && m_reference != LUA_REFNIL); }
    void SetLuaState(LuaState* pState) { m_pState = pState; }
    LuaState* GetLuaState() const { return m_pState; }
    void SetLuaStateToDefault() { m_pState = s_pDefaultLuaState; }

    // sets the LuaVar value, overwriting any existing value (and type)
    template <typename IntType = LuaInt> void SetInteger(IntType val);
    template <typename FloatType = LuaFloat> void SetNumber(FloatType val);
    void SetString(const char* val);
    void SetNil();
    void SetBool(bool val);
    void SetLightUserData(void* pVal);
    template <class Type> void SetValue(Type value);

    // attempts to convert the current value to the appropriate type and returns it
    template <typename IntType = LuaInt> IntType GetInteger() const;
    template <typename FloatType = LuaFloat> FloatType GetNumber() const;
    const char* GetString() const;  // Important!  This will convert the item to a lua string!
    bool GetBool() const;
    void* GetLightUserData() const;
    void* GetUserData() const;
    template <class Type> Type GetValue() const;

    // returns true if the value is of the given type, false if not
    bool IsInteger() const;
    bool IsNumber() const;
    bool IsString() const;
    bool IsNil() const;
    bool IsBool() const;
    bool IsLightUserData() const;
    bool IsUserData() const;
    bool IsFunction() const;
    bool IsCFunction() const;
    bool IsTable() const;
    template <class Type> bool IsType() const;

    // more type functions
    const char* GetTypeName() const;
    luastl::string GetTypeNameStr() const;

    // TODO: Lua's lua_isnumber() and lua_isstring() functions return whether or not the value is convertable to that 
    // type.  Consider whether or not we should have IsString() and IsNumber() return whether or not it's the exact 
    // type and leave these functions, or if we should have something like IsActuallyNumber() and IsActuallyString().
    // Alternatively, we can kill these functions and just check the Lua type.
    bool IsConvertableToNumber() const { return IsNumber(); }
    bool IsConvertableToString() const { return IsString(); }

    // Comparison operators.  These will compare vars as if you had done the comparison in Lua, including calling 
    // any metamethods.  Note that if you try to call a comparison operator like < on a table without an appropriate 
    // metamethod, you will get a hard crash (Lua panics and calls abort()).
    bool operator==(const LuaVar& right) const { return CompareHelper(right, LUA_OPEQ); }
    bool operator!=(const LuaVar& right) const { return !CompareHelper(right, LUA_OPEQ); }
    bool operator<(const LuaVar& right) const { return CompareHelper(right, LUA_OPLT); }
    bool operator<=(const LuaVar& right) const { return CompareHelper(right, LUA_OPLE); }
    bool operator>(const LuaVar& right) const { return !CompareHelper(right, LUA_OPLE); }
    bool operator>=(const LuaVar& right) const { return !CompareHelper(right, LUA_OPLT); }

    // table setters
    void SetTableVar(const char* key, const LuaVar& val) const;
    void SetTableVar(const char* key, LuaVar&& val) const;
    template <class IntType = int> void SetTableInteger(const char* key, IntType val) const;
    template <class FloatType = float> void SetTableNumber(const char* key, FloatType val) const;
    void SetTableString(const char* key, const char* val) const;
    void SetTableNil(const char* key) const;
    void SetTableBool(const char* key, bool val) const;
    void SetTableLightUserData(const char* key, void* pVal) const;
    template <class... Args> void FillTable(Args&&... args);
    LuaVar SetNewTable(const char* key, int nativeArraySize = 0, int hashSize = 0) const;  // sets this key to a newly created table and returns it
    void SetNewTableNoReturn(const char* key, int nativeArraySize = 0, int hashSize = 0) const;  // sets this key to a newly created table without returning it
    LuaVar InsertNewTableAtEnd(int nativeArraySize = 0, int hashSize = 0) const;  // pushes a newly created table to the end of this array and returns it
    void InserNewTableAtEndNoReturn(int nativeArraySize = 0, int hashSize = 0) const;  // pushes a newly created table to the end of this array without returning it

    // table getters
    LuaVar GetTableVar(const char* key) const;
    template <class IntType = int> IntType GetTableInteger(const char* key) const;
    template <class FloatType = int> FloatType GetTableNumber(const char* key) const;
    const char* GetTableString(const char* key) const;
    bool GetTableBool(const char* key) const;
    void* GetTableLightUserData(const char* key) const;
    void* GetTableUserData(const char* key) const;
    template <class Type> Type GetTableValue(const char* key) const;
    template <class Type> void SetTableValue(const char* key, Type value) const;
    LuaVar GetOrCreateNewTable(const char* key, int nativeArraySize = 0, int hashSize = 0) const;

    template <class Type> void Insert(size_t pos, Type val) const;  // behaves like table.insert()
    template <class Type> void Insert(Type val) const;  // pushes to the end, like table.insert()

    // special table functions
    void CreateNewTable(int nativeArraySize = 0, int hashSize = 0);  // creates a new table and points this variable to it
    TableIterator begin();
    TableIterator end();
    TableIterator begin() const;
    TableIterator end() const;
    LuaVar Lookup(const luastl::string& path) const;
    size_t GetLength() const;  // works for strings, tables, and userdata
    size_t GetNumElements() const;

    // indexing into tables
    template <class RetType, class IndexType> RetType GetAt(IndexType) const;
    template <class IndexType> LuaVar GetVarAt(IndexType index) const;
    //LuaVar operator[](size_t index)                 { return GetVarAt(index); }
    //LuaVar operator[](const char* index)            { return GetVarAt(index); }  // this causes a conflict when using an int, like foo[0]; not worth it
    LuaVar operator[](size_t index) const           { LUA_ASSERT(index > 0); return GetVarAt(index); }  // assert for a 0 index since Lua is 1-indexed
    LuaVar operator[](const char* index) const      { return GetVarAt(index); }
    //LuaVar operator[](const LuaVar& index)          { return GetVarAt(index); }
    LuaVar operator[](const LuaVar& index) const    { return GetVarAt(index); }

    // metatable functions
    void SetMetaTable(const LuaVar& metaTable) const;
    LuaVar GetMetaTable() const;

    // special userdata functions
    void WrapObjectPtr(void* pPtr);

    // function registration
    template <class Func> void BindFunction(const char* name, Func&& func) const;  // binds a global C function or static class function
    template <class ObjType, class Func> void BindFunction(const char* name, ObjType* pObj, Func&& func) const;  // binds a member function, using pObj as the "this" pointer
    template <class ObjType, class Func> void BindFunction(const char* name, Func&& func) const;  // binds a member function whose "this" pointer will be resolved at call time

    // stack functions
    bool PushValueToStack(bool allowNil = true) const;

private:
    void CreateRegisteryEntryFromStack();
    bool CompareHelper(const LuaVar& right, int op) const;

    // performs a Lua action on the object
    template <class Func> auto DoLuaAction(Func func, bool allowNil = false) const -> decltype(func());

    // copy/move functions
    void Copy(const LuaVar& right);
    void Move(LuaVar&& right) noexcept;

    // internal iterator helpers
    TableIterator InternalBegin() const;
    TableIterator InternalEnd() const;

    // table filling helper
    template <class KeyType, class ValueType, class... Args> void FillTableHelper(KeyType&& keyType, ValueType&& valueType, Args&&... args);

    // global function registration helpers
    template <class Func> static int CallBoundFunction(lua_State* pState);  // for C functions or static functions
    template <class RetType, class... Args> static int Call(lua_State* pState, RetType(*pFunc)(Args... args));

    // member function registration helpers
    template <class Obj, class Func> static int CallBoundMemberFunctionObjPair(lua_State* pState);  // for when the same object is always used
    template <class Obj, class Func> static int CallBoundMemberFunction(lua_State* pState);  // for when the object is passed in through the __object field
    template <class Obj, class RetType, class... Args> static int Call(lua_State* pState, Obj* pObj, RetType(Obj::*pFunc)(Args... args));
    template <class Obj, class RetType, class... Args> static int Call(lua_State* pState, const Obj* pObj, RetType(Obj::* pFunc)(Args... args) const);

    // helpers to build the arguments tuple
    template <class... Args> luastl::tuple<Args...> static BuildArguments(LuaState* pState);
    template <class Obj, class... Args> luastl::tuple<Obj*, Args...> static BuildArgumentsWithObj(LuaState* pState, Obj* pObj);
    template <size_t kTupleIndex, size_t kParamIndex, class TupleType, class Arg, class... Args> static void ProcessArg(LuaState* pState, TupleType& tupleToFill, size_t numArgs);
    template <size_t kTupleIndex, size_t kParamIndex, class TupleType> static void ProcessArg(LuaState* pState, TupleType& tupleToFill, size_t numArgs);
};

//---------------------------------------------------------------------------------------------------------------------
// The LuaVar specializations of these stack helpers require us to know what a LuaVar is.  This creates a circular 
// include reference, so we need to define them here.
//---------------------------------------------------------------------------------------------------------------------
namespace StackHelpers  {

// Push()
template <class Type>
luastl::enable_if_t<luastl::is_same<Type, LuaVar>::value> Push([[maybe_unused]] LuaState* pState, Type val)
{
    val.PushValueToStack();
}

// Get()
template <class Type>
luastl::enable_if_t<luastl::is_same<Type, LuaVar>::value, Type> Get(LuaState* pState, int stackIndex = -1)
{
    // Duplicate the value we're grabbing.  This is a hack to ensure that the behavior of all the templated Get() 
    // functions remains consistent.  The problem is that none of the other functions modify the stack but the 
    // CreateFromStack() function does since it calls LuaL_ref().  This will allow the stack's state to be the 
    // same before and after this call, though we are technically modifying the stack.
    lua_pushvalue(pState->GetState(), stackIndex);

    return LuaVar::CreateFromStack(pState);
}

// Is()
template <class Type>
luastl::enable_if_t<luastl::is_same<Type, LuaVar>::value, bool> Is([[maybe_unused]] LuaState* pState, [[maybe_unused]] int stackIndex = -1)
{
    return true;  // everything is a valid LuaVar
}

// GetDefault()
template <class Type>
luastl::enable_if_t<luastl::is_same<Type, LuaVar>::value, Type> GetDefault()
{
    return LuaVar();
}

// This specialization of GetFromStack() is exctly the same as the generic one in StackHelpers.h, except for the 
// explicit types.  We have to do this because GetFromStack() has no idea what LuaVar is, so calling it with a 
// LuaVar will fail to compile.
template <>
inline LuaVar GetFromStack<LuaVar>(LuaState* pState, [[maybe_unused]] bool showErrorOnUnderrun /*= true*/)
{
    LUA_ASSERT(pState);

    if (lua_gettop(pState->GetState()) > 0)
    {
        LuaVar value = Get<LuaVar>(pState);
        lua_pop(pState->GetState(), 1);  // make sure we pop since we have no idea how many parameters there are
        return value;
    }
    else
    {
#if BLEACHLUA_DEBUG_MODE
        if (showErrorOnUnderrun)
            LUA_ERROR("Lua stack underrun: Trying to pop value from empty stack.");
#endif
        return LuaVar();
    }
}

}  // end namespace BleachLua::StackHelpers


//---------------------------------------------------------------------------------------------------------------------
// These set this variable to the appropriate value.
//      -val:   The val to set.
//---------------------------------------------------------------------------------------------------------------------
template <typename IntType>
void LuaVar::SetInteger(IntType val)
{
    static_assert(luastl::is_integral<IntType>::value, "SetInteger() requires an integral type.");
    SetValue(static_cast<lua_Integer>(val));
}

template <typename FloatType>
void LuaVar::SetNumber(FloatType val)
{
    static_assert(luastl::is_floating_point<FloatType>::value, "SetNumber() requires a floating point type.");
    SetValue(static_cast<lua_Number>(val));
}

//---------------------------------------------------------------------------------------------------------------------
// These get this variable's value as the appropriate type.
//      -retturn:   The value stored in this variable.
//---------------------------------------------------------------------------------------------------------------------
template <typename IntType>
IntType LuaVar::GetInteger() const
{
    static_assert(luastl::is_integral<IntType>::value, "GetInteger() requires an integral type.");
    return static_cast<IntType>(GetValue<lua_Integer>());
}

template <typename FloatType>
FloatType LuaVar::GetNumber() const
{
    static_assert(luastl::is_floating_point<FloatType>::value, "GetNumber() requires a floating point type.");
    return static_cast<FloatType>(GetValue<lua_Number>());
}

//---------------------------------------------------------------------------------------------------------------------
// Sets a field in this table to the given value.
// IMPORTANT: This variable must be a table.
//      -key:   The key of the field.
//      -val:   The value to set.
//---------------------------------------------------------------------------------------------------------------------
template <class IntType>
void LuaVar::SetTableInteger(const char* key, IntType val) const
{
    static_assert(luastl::is_integral<IntType>::value, "SetTableInteger() requires an integral type.");
    SetTableValue<lua_Integer>(key, static_cast<lua_Integer>(val));
}

template <class FloatType>
void LuaVar::SetTableNumber(const char* key, FloatType val) const
{
    static_assert(luastl::is_floating_point<FloatType>::value, "SetTableNumber() requires a floating point type.");
    SetTableValue<lua_Number>(key, static_cast<lua_Number>(val));
}

//---------------------------------------------------------------------------------------------------------------------
// 
//---------------------------------------------------------------------------------------------------------------------
template <class... Args>
void LuaVar::FillTable(Args&&... args)
{
    static_assert((sizeof...(Args) % 2) == 0, "The number of arguments in FillTable() must be even.");
    FillTableHelper(std::forward<Args>(args)...);
}

//---------------------------------------------------------------------------------------------------------------------
// Gets a field in this table to the given value.
// IMPORTANT: This variable must be a table.
//      -key:       The key of the field.
//      -return:    The value at that field.  Note that if the variable isn't of the appropriate type, it will return 
//                  whatever the Lua runtime returns in those instances.
//---------------------------------------------------------------------------------------------------------------------
template <class IntType>
IntType LuaVar::GetTableInteger(const char* key) const
{
    static_assert(luastl::is_integral<IntType>::value, "GetTableInteger() requires an integral type.");
    return static_cast<IntType>(GetTableValue<lua_Integer>(key));
}

template <class FloatType>
FloatType LuaVar::GetTableNumber(const char* key) const
{
    static_assert(luastl::is_floating_point<FloatType>::value, "GetTableNumber() requires a floating point type.");
    return static_cast<FloatType>(GetTableValue<lua_Number>(key));
}

//---------------------------------------------------------------------------------------------------------------------
// Inserts the value into the table at the specified position.  In the overload that has no posiiton, it will append 
// to end.  This generally mirrors the behavior of table.insert().
//      -pos:   The index to insert this item into.
//      -val:   The value to insert.
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
void LuaVar::Insert([[maybe_unused]] size_t pos, [[maybe_unused]] Type val) const
{
    LUA_ERROR("Not implemented.");
}

template <class Type>
void LuaVar::Insert(Type val) const
{
    if (!IsTable())
    {
        LUA_ERROR("Attempting to insert a value into a LuaVar that is not a table.  Type is " + GetTypeNameStr());
        return;
    }

    DoLuaAction([this, val]()
    {
        const int index = static_cast<int>(GetLength() + 1);

#if BLEACHLUA_CORE_VERSION >= 53                        //  [t]
        StackHelpers::Push<Type>(m_pState, val);        //  [t, val]
        lua_seti(m_pState->GetState(), -2, index);      //  [t]
#else                                                   //  [t]
        StackHelpers::Push(m_pState, index);            //  [t, index]
        StackHelpers::Push<Type>(m_pState, val);        //  [t, index, val]
        lua_settable(m_pState->GetState(), -3);         //  [t]
#endif

    });
}

//---------------------------------------------------------------------------------------------------------------------
// Returns a value at the given index.
//      -index:     The index of the object, using Lua indexes (so int-based indexes start at 1).
//      -return:    The value of the returned object.
//---------------------------------------------------------------------------------------------------------------------
template <class RetType, class IndexType>
RetType LuaVar::GetAt(IndexType index) const
{
    LuaVar var = GetVarAt(index);
    return var.GetValue<RetType>();
}

//---------------------------------------------------------------------------------------------------------------------
// Returns a LuaVar at the given index.
//      -index:     The index of the object, using Lua indexes (so int-based indexes start at 1).
//      -return:    The value of the returned object.
//---------------------------------------------------------------------------------------------------------------------
template <class IndexType>
LuaVar LuaVar::GetVarAt(IndexType index) const
{
    static_assert(IsLuaString<IndexType>::value || IsLuaInteger<IndexType>::value || luastl::is_same<IndexType, LuaVar>::value, "GetVarAt() requires a string, integral type, or LuaVar as its index paramter.");

    return DoLuaAction([this, &index]() -> LuaVar
    {
        StackHelpers::Push(m_pState, index);        // [t, index]
        lua_gettable(m_pState->GetState(), -2);     // [t, val]
        return CreateFromStack(m_pState);           // [t]
    });                                             // []
}

//---------------------------------------------------------------------------------------------------------------------
// Binds a C function to this table by name.  Note that this variable must be a table.
//      -name:  The key to insert this function into.  Assuming this table is named Foo in Lua, you would call this 
//              as Foo.name();
//      -func:  The C function to bind.  The function can take any number of parameters, as long as they are 
//              convertable to Lua types.
//---------------------------------------------------------------------------------------------------------------------
template <class Func>
void LuaVar::BindFunction(const char* name, Func&& func) const
{
    LUA_ASSERT(m_pState);
    LUA_ASSERT_MSG(IsTable(), "LuaVar must be a table, type is " + GetTypeNameStr());

    // get the table from the registry and push it to the stack                         // Stack, from bottom to top:
    PushValueToStack();                                                                 //  [t]
    lua_pushlightuserdata(m_pState->GetState(), std::forward<Func>(func));              //  [t, func]
    lua_pushcclosure(m_pState->GetState(), &CallBoundFunction<Func>, 1);                //  [t, closure]
    lua_setfield(m_pState->GetState(), -2, name);  // essentially t[name] = closure     //  [t]
    lua_pop(m_pState->GetState(), 1);                                                   //  []
}

//---------------------------------------------------------------------------------------------------------------------
// Binds a C++ member function and instance object to this table by name.  Note that this variable must be a table.
// Any call to this function will call call the bound member function on the bound object.  Note that if you attempt 
// to call this function after the object has been destroyed, you will get a crash.  There's no safety checking.
// TODO: Consider having a WeakPtr version of this function.
//      -name:  The key to insert this function into.  Assuming this table is named Foo in Lua, you would call this 
//              as Foo.name();
//      -pObj:  The C++ instance object that will be the "this" of the function call.
//      -func:  The C++ member function to bind.  The function can take any number of parameters, as long as they are 
//              convertable to Lua types.
//---------------------------------------------------------------------------------------------------------------------
template <class ObjType, class Func>
void LuaVar::BindFunction(const char* name, ObjType* pObj, Func&& func) const
{
    LUA_ASSERT(m_pState);
    LUA_ASSERT_MSG(IsTable(), "LuaVar must be a table, type is " + GetTypeNameStr());

    // get the table from the registry and push it to the stack                                     // Stack, from bottom to top:
    PushValueToStack();                                                                             //  [t]
    lua_pushlightuserdata(m_pState->GetState(), pObj);                                              //  [t, pObj]

    // HACK: Pointers to member functions can't be converted into raw pointers, so we can't just store a userdata 
    // value in the closure like we do for C functions.  The hacky workaround is to create a new userdata buffer 
    // and memcpy() the function into it.  This seems super dangerous and probably not something that's officially 
    // supported, but it does work with MSVC.
    unsigned char* pBuffer = (unsigned char*)lua_newuserdata(m_pState->GetState(), sizeof(Func));   //  [t, pObj, buffer]
    memcpy(pBuffer, &func, sizeof(Func));

    lua_pushcclosure(m_pState->GetState(), &CallBoundMemberFunctionObjPair<ObjType, Func>, 2);      //  [t, closure]
    lua_setfield(m_pState->GetState(), -2, name);  // essentially t[name] = closure                 //  [t]
    lua_pop(m_pState->GetState(), 1);                                                               //  []
}

//---------------------------------------------------------------------------------------------------------------------
// Binds a C++ member function to this table by name.  The object used as the "this" pointer is resolved at runtime 
// by looking at the Lua object's __object member.  __object must be of type ObjType or behavior is undefined.
//      -name:  The key to insert this function into.  Assuming this table is named Foo in Lua, you would call this 
//              as Foo:name(), assuming Foo.__object contains a valid C++ pointer to the appropriate ObjType.
//      -func:  The C++ member function to bind.  The function can take any number of parameters, as long as they are 
//              convertable to Lua types.
//---------------------------------------------------------------------------------------------------------------------
template <class ObjType, class Func>
void LuaVar::BindFunction(const char* name, Func&& func) const
{
    LUA_ASSERT(m_pState);
    LUA_ASSERT_MSG(IsTable(), "LuaVar must be a table, type is " + GetTypeNameStr());

    // get the table from the registry and push it to the stack                                     // Stack, from bottom to top:
    PushValueToStack();                                                                             //  [t]

    // HACK: Pointers to member functions can't be converted into raw pointers, so we can't just store a userdata 
    // value in the closure like we do for C functions.  The hacky workaround is to create a new userdata buffer 
    // and memcpy() the function into it.  This seems super dangerous and probably not something that's officially 
    // supported, but it does work with MSVC.
    unsigned char* pBuffer = (unsigned char*)lua_newuserdata(m_pState->GetState(), sizeof(Func));   //  [t, buffer]
    memcpy(pBuffer, &func, sizeof(Func));

    lua_pushcclosure(m_pState->GetState(), &CallBoundMemberFunction<ObjType, Func>, 1);             //  [t, closure]
    lua_setfield(m_pState->GetState(), -2, name);  // essentially t[name] = closure                 //  [t]
    lua_pop(m_pState->GetState(), 1);                                                               //  []
}

//---------------------------------------------------------------------------------------------------------------------
// Performs a Lua action on the variable that we represent.  These functions basically wrap all the stack 
// manipulation.  There are two versions, one for functions with a return value and one for functions with a void 
// return.
//      -func:      The function to call.  It should take no parameters but may return something if desired.  When 
//                  func gets called, this variable will be at the top of the stack.  It is automatically removed 
//                  at the end, so no need to pop it.
//      -return:    Whatever the passed-in function returns.  Note that if the passed-in function has a void return, 
//                  the void-return version of this function is used.
//                  TODO: Replace the return value with luastl::optional.
//---------------------------------------------------------------------------------------------------------------------
template <class Func>
auto LuaVar::DoLuaAction(Func func, bool allowNil /*= false*/) const -> decltype(func())
{
    LUA_ASSERT(m_pState);

    // Grab the value from the registry if it's valid.  If not, and we want to allow nil values, push a nil value 
    // to the stack.  Otherwise, we do nothing.
    if (!PushValueToStack(allowNil))
    {
        if constexpr (luastl::is_void_v<decltype(func())>)
            return;
        else
            return {};  // return a zero-initialized value
    }

    // If we get here, the top of the stack should hold our value.
    if constexpr (luastl::is_void_v<decltype(func())>)
    {
        func();
        lua_pop(m_pState->GetState(), 1);
    }
    else
    {
        auto ret = func();
        lua_pop(m_pState->GetState(), 1);

        return ret;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Private work-horse for the various Get***() functions.
//      -returns:   Returns the Lua value represented by this variable.  If it's not the appropriate type, it will 
//                  return whatever the appropriate lua_to***() function returns.
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
Type LuaVar::GetValue() const
{
    LUA_ASSERT(m_pState);
    LUA_ASSERT(m_reference != LUA_REFNIL);
    return DoLuaAction([this]() -> Type { return StackHelpers::Get<Type>(m_pState); });
}

//---------------------------------------------------------------------------------------------------------------------
// Private work-horse for the various Set***() functions.  The nullptr_t specialization is to handle the case where 
// you assign nil.  We don't want to push anything to the stack or deal with that, so we just clear the reference.
// This has the same effect.
//      -value: The value to set.  This must a Lua-convertable type.
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
void LuaVar::SetValue(Type value)
{
    LUA_ASSERT(m_pState);
    StackHelpers::Push<Type>(m_pState, value);
    CreateRegisteryEntryFromStack();
}

template <>
inline void LuaVar::SetValue<nullptr_t>(nullptr_t)
{
    ClearRef();
}

//---------------------------------------------------------------------------------------------------------------------
// Private work-horse for the various GetTable***() functions.
//      -key:       The key in this table.
//      -returns:   Returns the Lua value represented by this variable.  If it's not the appropriate type, it will 
//                  return whatever the appropriate lua_to***() function returns.
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
Type LuaVar::GetTableValue(const char* key) const
{
    LUA_ASSERT(m_pState);

    if (!IsTable())
    {
        LUA_ERROR("Trying to get a table value from a var that isn't a table.  Type is " + GetTypeNameStr());
        return StackHelpers::GetDefault<Type>();
    }
    
    // get the table from the registry and push it to the stack
    PushValueToStack();                                                         // [t]

    // push the string key to the stack and then get the value from the table using that string key
    lua_pushstring(m_pState->GetState(), key);                                  // [t, key]
    lua_gettable(m_pState->GetState(), -2);                                     // [t, value]

    // do some type checking
    if (!StackHelpers::Is<Type>(m_pState))
    {
        LUA_ERROR("Trying to get key " + luastl::string(key) + " but it's not of the appropriate type.");
        lua_pop(m_pState->GetState(), 2);                                       // []
        return StackHelpers::GetDefault<Type>();
    }

    // grab the result from the table and push and pop the stack
    Type result = StackHelpers::Get<Type>(m_pState);                            // [t, value]
    lua_pop(m_pState->GetState(), 2);                                           // []

    return result;
}

//---------------------------------------------------------------------------------------------------------------------
// Private work-horse for the various SetTable***() functions.
//      -key:       The key in this table.
//      -value:     The value to set.  This must a Lua-convertable type.
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
void LuaVar::SetTableValue(const char* key, Type value) const
{
    LUA_ASSERT(m_pState);

    if (!IsTable())
    {
        LUA_ERROR("Trying to set a table value on a var that isn't a table.  Type is " + GetTypeNameStr());
        return;
    }

    // get the table from the registry and push it to the stack
    PushValueToStack();                                                         // [t]

    // push the string key and value onto the stack
    lua_pushstring(m_pState->GetState(), key);                                  // [t, key]
    StackHelpers::Push(m_pState, value);                                        // [t, key, val]

    // update the table with the new value
    lua_settable(m_pState->GetState(), -3);                                     // [t]
    lua_pop(m_pState->GetState(), 1);                                           // []
}

//---------------------------------------------------------------------------------------------------------------------
// Returns true if this variable is of the templated type.  Type can be anything, but if it's not a Lua-convertable 
// type, it's guaranteed to return false.
//      -return:    true if this variable is of the templated type, false if not.
//---------------------------------------------------------------------------------------------------------------------
template <class Type>
bool LuaVar::IsType() const
{
    // Check for nil first.  There are many cases where we have an uninitialized LuaVar that we want to treat as 
    // nil, so we do a special check here to avoid having to care if m_pState is valid.  IsNil() has it's own 
    // implementation, so it's not recursively calling this function.  This also means that if this variable is 
    // nil, it automatically fails the type check.
    if (IsNil())
        return false;
    return DoLuaAction([this]() -> bool { return StackHelpers::Is<Type>(m_pState); });
}

//---------------------------------------------------------------------------------------------------------------------
// Worker function to fill a table.
//---------------------------------------------------------------------------------------------------------------------
template <class KeyType, class ValueType, class... Args>
void LuaVar::FillTableHelper(KeyType&& keyType, ValueType&& valueType, Args&&... args)
{
    static_assert(luastl::is_convertible<KeyType, const char*>::value, "KeyType must be a C-style string.");
    SetTableValue(keyType, std::forward<ValueType>(valueType));
    if constexpr (sizeof...(Args) > 0)
        FillTableHelper(std::forward<Args>(args)...);
}

//---------------------------------------------------------------------------------------------------------------------
// Helper function to call the bound function.  This is the actual Lua C function that is bound to the string.  Its 
// job is to call the real function.
//      -pState:    The lua_State object to use.
//      -return:    The number of values returned.  Since this is a bound C function, it will only ever be 1 or 0.
//---------------------------------------------------------------------------------------------------------------------
template <class Func>
int LuaVar::CallBoundFunction(lua_State* pState)
{
    LUA_ASSERT(pState);

#if BLEACHLUA_DEBUG_MODE
    // This string will contain the Lua stack trace that called into this function.  This exists only in debug mode 
    // and is never used anywhere.  The purpose is so that you can look at the value in the debugger and see what 
    // Lua call brought you here.
    [[maybe_unused]] luastl::string traceback = Debug::GetTraceback(GetCppStateFromCState(pState));
#endif

    Func pFunc = static_cast<Func>(lua_touserdata(pState, lua_upvalueindex(1)));
    return LuaVar::Call(pState, pFunc);
}

//---------------------------------------------------------------------------------------------------------------------
// Helper function to call the bound function/object pair.  This is the actual Lua C function that is bound to the 
// string.  Its job is to call the real function, passing in the bound Obj pointer as the "this" pointer.
//      -pState:    The lua_State object to use.
//      -return:    The number of values returned.  Since this is a bound C function, it will only ever be 1 or 0.
//---------------------------------------------------------------------------------------------------------------------
template <class Obj, class Func>
int LuaVar::CallBoundMemberFunctionObjPair(lua_State* pState)
{
    LUA_ASSERT(pState);

#if BLEACHLUA_DEBUG_MODE
    // This string will contain the Lua stack trace that called into this function.  This exists only in debug mode 
    // and is never used anywhere.  The purpose is so that you can look at the value in the debugger and see what 
    // Lua call brought you here.
    [[maybe_unused]] luastl::string traceback = Debug::GetTraceback(GetCppStateFromCState(pState));
#endif

    Obj* pObj = static_cast<Obj*>(lua_touserdata(pState, lua_upvalueindex(1)));

    void* pFuncBuffer = lua_touserdata(pState, lua_upvalueindex(2));
    Func* pFunc = reinterpret_cast<Func*>(pFuncBuffer);
    return LuaVar::Call<Obj>(pState, pObj, *pFunc);
}

//---------------------------------------------------------------------------------------------------------------------
// Helper function to call the bound member function.  The first parameter of the function is assumed to be a table 
// with an __object member that is a valid Obj pointer.  This will be used as the "this" pointer.  Note that this 
// table is not passed in as a parameter to the bound function and is popped from the stack before calling the true 
// function.
//      -pState:    The lua_State object to use.
//      -return:    The number of values returned.  Since this is a bound C function, it will only ever be 1 or 0.
//---------------------------------------------------------------------------------------------------------------------
template <class Obj, class Func>
int LuaVar::CallBoundMemberFunction(lua_State* pState)
{
    LUA_ASSERT(pState);

#if BLEACHLUA_DEBUG_MODE
    // This string will contain the Lua stack trace that called into this function.  This exists only in debug mode 
    // and is never used anywhere.  The purpose is so that you can look at the value in the debugger and see what 
    // Lua call brought you here.
    [[maybe_unused]] luastl::string traceback = Debug::GetTraceback(GetCppStateFromCState(pState));
#endif

    // make sure we have a table
    int type = lua_type(pState, 1);
    if (type != LUA_TTABLE)
    {
        LUA_ERROR("No table was passed into bound function.  Type is " + luastl::string(lua_typename(pState, type)));
        return 0;  // note: this will fall immediately back to Lua so we don't need to care about cleaning up the stack
    }

    // Push the table so we don't modify it, then push the string we want.  Grab the __object member of 
    // the passed in table.
    lua_pushvalue(pState, 1);                           //  [t, params, t]
    lua_pushstring(pState, "__object");                 //  [t, params, t, "__object"]
    lua_rawget(pState, -2);                             //  [t, params, t, __object]

    // make sure __object is a valid userdatum
    if (!lua_isuserdata(pState, -1))
    {
        LUA_ERROR("Couldn't find userdata __object on parameter passed in.  Type is " + luastl::string(lua_typename(pState, lua_type(pState, -1))));
        return 0;  // note: this will fall immediately back to Lua so we don't need to care about cleaning up the stack
    }

    // cast to the appropriate object
    Obj* pObj = static_cast<Obj*>(lua_touserdata(pState, -1));

    // Fixup the stack.  We need to pop the temp table and the __object value from the stack and surgically remove 
    // the table that was passed in as the first parameter.  This leaves us with just the function parameters.
    lua_pop(pState, 2);                                 //  [t, params]
    lua_remove(pState, 1);                              //  [params]

    // grab the function
    void* pFuncBuffer = lua_touserdata(pState, lua_upvalueindex(1));
    Func* pFunc = reinterpret_cast<Func*>(pFuncBuffer);
    return LuaVar::Call<Obj>(pState, pObj, *pFunc);     //  [return]
}

//---------------------------------------------------------------------------------------------------------------------
// This function performs the actual call of the bound C function, getting all of the appropriate values from the 
// stack.  Note that there is currently no validation, so if you pass in the wrong types, you might be undefined 
// behavior.
// 
// Note that there are two versions of this function, one for void returns and one for non-void returns.
//      -pFunc:     The function to call.
//      -return:    Either the return value of the function or nothing.  If pFunc has a void return value, so will 
//                  this function.
//---------------------------------------------------------------------------------------------------------------------
template <class RetType, class... Args>
int LuaVar::Call(lua_State* pState, RetType(*pFunc)(Args... args))
{
    LuaState* pCppState = GetCppStateFromCState(pState);

    if constexpr (luastl::is_void_v<RetType>)
    {
        luastl::apply(pFunc, BuildArguments<Args...>(pCppState));
        return 0;
    }
    else
    {
        RetType val = luastl::apply(pFunc, BuildArguments<Args...>(pCppState));
        StackHelpers::Push(pCppState, val);
        return 1;
    }
}

template <class Obj, class RetType, class... Args>
int LuaVar::Call(lua_State* pState, Obj* pObj, RetType(Obj::*pFunc)(Args... args))
{
    LuaState* pCppState = GetCppStateFromCState(pState);

    if constexpr (luastl::is_void_v<RetType>)
    {
        luastl::apply(pFunc, BuildArgumentsWithObj<Obj, Args...>(pCppState, pObj));
        return 0;
    }
    else
    {
        RetType val = luastl::apply(pFunc, BuildArgumentsWithObj<Obj, Args...>(pCppState, pObj));
        StackHelpers::Push(pCppState, val);
        return 1;
    }
}

template <class Obj, class RetType, class... Args>
int LuaVar::Call(lua_State* pState, const Obj* pObj, RetType(Obj::* pFunc)(Args... args) const)
{
    LuaState* pCppState = GetCppStateFromCState(pState);

    if constexpr (luastl::is_void_v<RetType>)
    {
        luastl::apply(pFunc, BuildArgumentsWithObj<const Obj, Args...>(pCppState, pObj));
        return 0;
    }
    else
    {
        RetType val = luastl::apply(pFunc, BuildArgumentsWithObj<const Obj, Args...>(pCppState, pObj));
        StackHelpers::Push(pCppState, val);
        return 1;
    }
}

//---------------------------------------------------------------------------------------------------------------------
// Pulls the bound function arguments off the stack and builds them into a tuple.
//      -pState:    The LuaState object.
//      -return:    The tuple of arguments.
//---------------------------------------------------------------------------------------------------------------------
template <class... Args>
luastl::tuple<Args...> LuaVar::BuildArguments(LuaState* pState)
{                                                                                                                                   //  [args...]
    // recurssively process each argument
    luastl::tuple<Args...> tupleArgs;
    ProcessArg<0, 1, luastl::tuple<Args...>, Args...>(pState, tupleArgs, static_cast<size_t>(lua_gettop(pState->GetState())));

    // Clear the stack.  This should be safe since the only values on the stack should be the arguments to the 
    // function.  The function itself hasn't been called yet, so we don't have to worry about the return value.
    lua_settop(pState->GetState(), 0);                                                                                              //  []

    return tupleArgs;
}

//---------------------------------------------------------------------------------------------------------------------
// Pulls the bound function arguments off the stack and builds them into a tuple.
//      -pState:    The LuaState object.
//      -return:    The tuple of arguments.
//---------------------------------------------------------------------------------------------------------------------
template <class Obj, class... Args>
luastl::tuple<Obj*, Args...> LuaVar::BuildArgumentsWithObj(LuaState* pState, Obj* pObj)
{                                                                                                                                   //  [args...]
    using TupleType = luastl::tuple<Obj*, Args...>;

    // recurssively process each argument
    TupleType tupleArgs;
    luastl::get<0>(tupleArgs) = pObj;
    ProcessArg<1, 1, TupleType, Args...>(pState, tupleArgs, static_cast<size_t>(lua_gettop(pState->GetState())) + 1);  // +1 because we have to account for pObj

    // Clear the stack.  This should be safe since the only values on the stack should be the arguments to the 
    // function.  The function itself hasn't been called yet, so we don't have to worry about the return value.
    lua_settop(pState->GetState(), 0);                                                                                              //  []

    return tupleArgs;
}

//---------------------------------------------------------------------------------------------------------------------
// Helper function to recursively process each bound function argument and pull them into the tuple.  The first 
// specialization grabs the argument and recurses, the second one stops recursion.
//      -pState:        The LuaState object.
//      -tupleToFill:   A reference to the tuple we need to fill.
//      -numArgs:       The number of arguments passed to the function when it was called from Lua.
//---------------------------------------------------------------------------------------------------------------------
template <size_t kTupleIndex, size_t kParamIndex, class TupleType, class Arg, class... Args>
void LuaVar::ProcessArg(LuaState* pState, TupleType& tupleToFill, size_t numArgs)
{
    // If our index is less than the number of args we received, pull the value from the stack.  Otherwise, it means 
    // we didn't receive enough arguments.  This is valid in Lua but not C++.  We handle it by retrieving the default 
    // value for that type and passing it in.  Unfortunately, there is no way for C++ to know if an arg was default 
    // or if it was legitimately passed through, nor is there a way to change the default.  If you need to know 
    // whether or not the arg was actually passed in, your function will need to take a LuaVar as a parameter so you 
    // can check for nil.
    if (kTupleIndex < numArgs)
        luastl::get<kTupleIndex>(tupleToFill) = StackHelpers::Get<Arg>(pState, kParamIndex);
    else
        luastl::get<kTupleIndex>(tupleToFill) = StackHelpers::GetDefault<Arg>();

    // recurssively move to the next item to process
    ProcessArg<kTupleIndex + 1, kParamIndex + 1, TupleType, Args...>(pState, tupleToFill, numArgs);
}

template <size_t kTupleIndex, size_t kParamIndex, class TupleType>
void LuaVar::ProcessArg([[maybe_unused]] LuaState* pState, [[maybe_unused]] TupleType& tupleToFill, [[maybe_unused]] size_t numArgs)
{
    //
}

}  // end namespace BleachLua
