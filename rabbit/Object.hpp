/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/ObjectType.hpp>
#include <rabbit/ObjectValue.hpp>

namespace rabbit {

	class Object {
		public:
			rabbit::ObjectType _type;
			rabbit::ObjectValue _unVal;
			
			int64_t& toInteger() {
				return _unVal.nInteger;
			}
			int64_t toInteger() const {
				return _unVal.nInteger;
			}
			float_t& toFloat() {
				return _unVal.fFloat;
			}
			float_t toFloat() const {
				return _unVal.fFloat;
			}
			rabbit::String*& toString() {
				return _unVal.pString;
			}
			const rabbit::String* toString() const {
				return _unVal.pString;
			}
			rabbit::Table*& toTable() {
				return _unVal.pTable;
			}
			const rabbit::Table* toTable() const {
				return _unVal.pTable;
			}
			rabbit::Array*& toArray() {
				return _unVal.pArray;
			}
			const rabbit::Array* toArray() const {
				return _unVal.pArray;
			}
			rabbit::FunctionProto*& toFunctionProto() {
				return _unVal.pFunctionProto;
			}
			const rabbit::FunctionProto* toFunctionProto() const {
				return _unVal.pFunctionProto;
			}
			rabbit::Closure*& toClosure() {
				return _unVal.pClosure;
			}
			const rabbit::Closure* toClosure() const {
				return _unVal.pClosure;
			}
			rabbit::Outer*& toOuter() {
				return _unVal.pOuter;
			}
			const rabbit::Outer* toOuter() const {
				return _unVal.pOuter;
			}
			rabbit::Generator*& toGenerator() {
				return _unVal.pGenerator;
			}
			const rabbit::Generator* toGenerator() const {
				return _unVal.pGenerator;
			}
			rabbit::NativeClosure*& toNativeClosure() {
				return _unVal.pNativeClosure;
			}
			const rabbit::NativeClosure* toNativeClosure() const {
				return _unVal.pNativeClosure;
			}
			rabbit::Class*& toClass() {
				return _unVal.pClass;
			}
			const rabbit::Class* toClass() const {
				return _unVal.pClass;
			}
			rabbit::Instance*& toInstance() {
				return _unVal.pInstance;
			}
			const rabbit::Instance* toInstance() const {
				return _unVal.pInstance;
			}
			rabbit::Delegable*& toDelegable() {
				return _unVal.pDelegable;
			}
			const rabbit::Delegable* toDelegable() const {
				return _unVal.pDelegable;
			}
			rabbit::UserPointer& toUserPointer() {
				return _unVal.pUserPointer;
			}
			const rabbit::UserPointer toUserPointer() const {
				return _unVal.pUserPointer;
			}
			rabbit::WeakRef*& toWeakRef() {
				return _unVal.pWeakRef;
			}
			const rabbit::WeakRef* toWeakRef() const {
				return _unVal.pWeakRef;
			}
			rabbit::VirtualMachine*& toVirtualMachine() {
				return _unVal.pThread;
			}
			const rabbit::VirtualMachine* toVirtualMachine() const {
				return _unVal.pThread;
			}
			rabbit::RefCounted*& toRefCounted() {
				return _unVal.pRefCounted;
			}
			const rabbit::RefCounted* toRefCounted() const {
				return _unVal.pRefCounted;
			}
			rabbit::UserData*& toUserData() {
				return _unVal.pUserData;
			}
			const rabbit::UserData* toUserData() const {
				return _unVal.pUserData;
			}
			uint64_t& toRaw() {
				return _unVal.raw;
			}
			uint64_t toRaw() const {
				return _unVal.raw;
			}
	};
	
	#define __addRef(type,unval) if(ISREFCOUNTED(type)) \
		{ \
			unval.pRefCounted->refCountIncrement(); \
		}
	
	#define __release(type,unval) if(ISREFCOUNTED(type) && (unval.pRefCounted->refCountDecrement()==0))  \
		{   \
			unval.pRefCounted->release();   \
		}
	
	#define _realval(o) (sq_type((o)) != rabbit::OT_WEAKREF?(rabbit::Object)o:_weakref(o)->_obj)
	
	#define is_delegable(t) (sq_type(t)&SQOBJECT_DELEGABLE)
	#define raw_type(obj) _RAW_TYPE((obj)._type)
	
	#define _closure(obj) ((obj)._unVal.pClosure)
	#define _generator(obj) ((obj)._unVal.pGenerator)
	#define _nativeclosure(obj) ((obj)._unVal.pNativeClosure)
	#define _userdata(obj) ((obj)._unVal.pUserData)
	#define _userpointer(obj) ((obj)._unVal.pUserPointer)
	#define _thread(obj) ((obj)._unVal.pThread)
	#define _funcproto(obj) ((obj)._unVal.pFunctionProto)
	#define _class(obj) ((obj)._unVal.pClass)
	#define _instance(obj) ((obj)._unVal.pInstance)
	#define _delegable(obj) ((rabbit::Delegable *)(obj)._unVal.pDelegable)
	#define _weakref(obj) ((obj)._unVal.pWeakRef)
	#define _outer(obj) ((obj)._unVal.pOuter)
	#define _refcounted(obj) ((obj)._unVal.pRefCounted)
	#define _rawval(obj) ((obj)._unVal.raw)
	
	#define _stringval(obj) (obj)._unVal.pString->_val
	#define _userdataval(obj) ((rabbit::UserPointer)sq_aligning((obj)._unVal.pUserData + 1))
	
	#define tofloat(num) ((sq_type(num)==rabbit::OT_INTEGER)?(float_t)(num).toInteger():(num).toFloat())
	#define tointeger(num) ((sq_type(num)==rabbit::OT_FLOAT)?(int64_t)(num).toFloat():(num).toInteger())
	
	
	#define sq_isnumeric(o) ((o)._type&SQOBJECT_NUMERIC)
	#define sq_istable(o) ((o)._type==rabbit::OT_TABLE)
	#define sq_isarray(o) ((o)._type==rabbit::OT_ARRAY)
	#define sq_isfunction(o) ((o)._type==rabbit::OT_FUNCPROTO)
	#define sq_isclosure(o) ((o)._type==rabbit::OT_CLOSURE)
	#define sq_isgenerator(o) ((o)._type==rabbit::OT_GENERATOR)
	#define sq_isnativeclosure(o) ((o)._type==rabbit::OT_NATIVECLOSURE)
	#define sq_isstring(o) ((o)._type==rabbit::OT_STRING)
	#define sq_isinteger(o) ((o)._type==rabbit::OT_INTEGER)
	#define sq_isfloat(o) ((o)._type==rabbit::OT_FLOAT)
	#define sq_isuserpointer(o) ((o)._type==rabbit::OT_USERPOINTER)
	#define sq_isuserdata(o) ((o)._type==rabbit::OT_USERDATA)
	#define sq_isthread(o) ((o)._type==rabbit::OT_THREAD)
	#define sq_isnull(o) ((o)._type==rabbit::OT_NULL)
	#define sq_isclass(o) ((o)._type==rabbit::OT_CLASS)
	#define sq_isinstance(o) ((o)._type==rabbit::OT_INSTANCE)
	#define sq_isbool(o) ((o)._type==rabbit::OT_BOOL)
	#define sq_isweakref(o) ((o)._type==rabbit::OT_WEAKREF)
	#define sq_type(o) ((o)._type)
	
	inline void _Swap(rabbit::Object &a,rabbit::Object &b)
	{
		rabbit::ObjectType tOldType = a._type;
		rabbit::ObjectValue unOldVal = a._unVal;
		a._type = b._type;
		a._unVal = b._unVal;
		b._type = tOldType;
		b._unVal = unOldVal;
	}
}
