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
#include <rabbit/sqconfig.hpp>
#include <rabbit/Object.hpp>

namespace rabbit {
	#define RABBIT_OBJ_REF_TYPE_DECLARE(type,_class,sym) \
		ObjectPtr(_class * x); \
		ObjectPtr& operator=(_class *x);
	
	#define RABBIT_SCALAR_TYPE_DECLARE(type,_class,sym) \
		ObjectPtr(_class x); \
		ObjectPtr& operator=(_class x);
	
	class ObjectPtr : public rabbit::Object {
		public:
			ObjectPtr();
			ObjectPtr(const ObjectPtr& _obj);
			ObjectPtr(const Object& _obj);
			
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_TABLE, rabbit::Table, pTable)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_CLASS, rabbit::Class, pClass)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_INSTANCE, rabbit::Instance, pInstance)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_ARRAY, rabbit::Array, pArray)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_CLOSURE, rabbit::Closure, pClosure)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_NATIVECLOSURE, rabbit::NativeClosure, pNativeClosure)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_OUTER, rabbit::Outer, pOuter)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_GENERATOR, rabbit::Generator, pGenerator)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_STRING, rabbit::String, pString)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_USERDATA, UserData, pUserData)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_WEAKREF, WeakRef, pWeakRef)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_THREAD, VirtualMachine, pThread)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_FUNCPROTO, rabbit::FunctionProto, pFunctionProto)
			
			RABBIT_SCALAR_TYPE_DECLARE(rabbit::OT_INTEGER, int64_t, nInteger)
			RABBIT_SCALAR_TYPE_DECLARE(rabbit::OT_FLOAT, float_t, fFloat)
			RABBIT_SCALAR_TYPE_DECLARE(rabbit::OT_USERPOINTER, UserPointer, pUserPointer)
			
			ObjectPtr(bool _value);
			ObjectPtr& operator=(bool _value);
			~ObjectPtr();
			ObjectPtr& operator=(const ObjectPtr& _obj);
			ObjectPtr& operator=(const Object& _obj);
			void Null();
		private:
			ObjectPtr(const char * _obj){} //safety
	};
	
	
	uint64_t translateIndex(const rabbit::ObjectPtr &idx);
	const char *getTypeName(const rabbit::ObjectPtr &obj1);
	const char *IdType2Name(rabbit::ObjectType type);

}
