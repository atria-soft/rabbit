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
			int64_t toIntegerValue() const {
				if (isInteger() == true){
					return _unVal.nInteger;
				}
				if (isFloat() == true){
					return _unVal.fFloat;
				}
				return 0;
			}
			float_t toFloatValue() const {
				if (isFloat() == true){
					return _unVal.fFloat;
				}
				if (isInteger() == true){
					return _unVal.nInteger;
				}
				return 0.0f;
			}
			rabbit::String*& toString() {
				return _unVal.pString;
			}
			const rabbit::String* toString() const {
				return _unVal.pString;
			}
			const char* getStringValue() const;
			char* getStringValue();
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
			bool isRefCounted() const {
				return (_type & SQOBJECT_REF_COUNTED) != 0;
			}
			bool isNumeric() const {
				return (_type & SQOBJECT_NUMERIC) != 0;
			}
			bool canBeFalse() const {
				return (_type & SQOBJECT_CANBEFALSE) != 0;
			}
			bool isDelegable() const {
				return (_type & SQOBJECT_DELEGABLE) != 0;
			}
			bool isTable() const {
				return _type == rabbit::OT_TABLE;
			}
			bool isArray() const {
				return _type == rabbit::OT_ARRAY;
			}
			bool isFunctionProto() const {
				return _type == rabbit::OT_FUNCPROTO;
			}
			bool isClosure() const {
				return _type == rabbit::OT_CLOSURE;
			}
			bool isGenerator() const {
				return _type == rabbit::OT_GENERATOR;
			}
			bool isNativeClosure() const {
				return _type == rabbit::OT_NATIVECLOSURE;
			}
			bool isString() const {
				return _type == rabbit::OT_STRING;
			}
			bool isInteger() const {
				return _type == rabbit::OT_INTEGER;
			}
			bool isFloat() const {
				return _type == rabbit::OT_FLOAT;
			}
			bool isUserPointer() const {
				return _type == rabbit::OT_USERPOINTER;
			}
			bool isUserData() const {
				return _type == rabbit::OT_USERDATA;
			}
			bool isVirtualMachine() const {
				return _type == rabbit::OT_THREAD;
			}
			bool isNull() const {
				return _type == rabbit::OT_NULL;
			}
			bool isClass() const {
				return _type == rabbit::OT_CLASS;
			}
			bool isInstance() const {
				return _type == rabbit::OT_INSTANCE;
			}
			bool isBoolean() const {
				return _type == rabbit::OT_BOOL;
			}
			bool isWeakRef() const {
				return _type == rabbit::OT_WEAKREF;
			}
			rabbit::ObjectType getType() const {
				return _type;
			}
			rabbit::ObjectType getTypeRaw() const {
				return rabbit::ObjectType(_type&_RT_MASK);
			}
			rabbit::Object getRealObject() const;
			rabbit::UserPointer getUserDataValue() const;
	};
	
	#define ISREFCOUNTED(t) (t&SQOBJECT_REF_COUNTED)
	
	#define __addRef(type,unval) if(ISREFCOUNTED(type)) \
		{ \
			unval.pRefCounted->refCountIncrement(); \
		}
	
	#define __release(type,unval) if(ISREFCOUNTED(type) && (unval.pRefCounted->refCountDecrement()==0))  \
		{   \
			unval.pRefCounted->release();   \
		}
	
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
