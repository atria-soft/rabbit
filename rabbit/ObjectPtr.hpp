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
	#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
		#define SQ_REFOBJECT_INIT() SQ_OBJECT_RAWINIT()
	#else
		#define SQ_REFOBJECT_INIT()
	#endif
	#define RABBIT_OBJ_REF_TYPE_DECLARE(type,_class,sym) \
		ObjectPtr(_class * x);\
		ObjectPtr& operator=(_class *x);
	
	#define RABBIT_SCALAR_TYPE_DECLARE(type,_class,sym) \
		ObjectPtr(_class x); \
		ObjectPtr& operator=(_class x);
	
	class ObjectPtr : public rabbit::Object {
		public:
			ObjectPtr();
			ObjectPtr(const ObjectPtr& _obj);
			ObjectPtr(const Object& _obj);
			
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_TABLE, SQTable, pTable)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_CLASS, rabbit::Class, pClass)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_INSTANCE, rabbit::Instance, pInstance)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_ARRAY, rabbit::Array, pArray)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_CLOSURE, SQClosure, pClosure)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_NATIVECLOSURE, SQNativeClosure, pNativeClosure)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_OUTER, SQOuter, pOuter)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_GENERATOR, SQGenerator, pGenerator)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_STRING, SQString, pString)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_USERDATA, UserData, pUserData)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_WEAKREF, WeakRef, pWeakRef)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_THREAD, VirtualMachine, pThread)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_FUNCPROTO, SQFunctionProto, pFunctionProto)
			
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
			ObjectPtr(const rabbit::Char * _obj){} //safety
	};
	
	
	uint64_t translateIndex(const rabbit::ObjectPtr &idx);
	const rabbit::Char *getTypeName(const rabbit::ObjectPtr &obj1);
	const rabbit::Char *IdType2Name(rabbit::ObjectType type);

}
