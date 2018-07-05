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
	#define RABBIT_OBJ_REF_TYPE_DECLARE(type,_class) \
		ObjectPtr(_class * x); \
		ObjectPtr& operator=(_class *x);
	
	#define RABBIT_SCALAR_TYPE_DECLARE(type,_class) \
		ObjectPtr(_class x); \
		ObjectPtr& operator=(_class x);
	
	class ObjectPtr : public rabbit::Object {
		public:
			ObjectPtr();
			ObjectPtr(const ObjectPtr& _obj);
			ObjectPtr(const Object& _obj);
			
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_TABLE, rabbit::Table)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_CLASS, rabbit::Class)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_INSTANCE, rabbit::Instance)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_ARRAY, rabbit::Array)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_CLOSURE, rabbit::Closure)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_NATIVECLOSURE, rabbit::NativeClosure)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_OUTER, rabbit::Outer)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_GENERATOR, rabbit::Generator)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_STRING, rabbit::String)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_USERDATA, UserData)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_WEAKREF, WeakRef)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_THREAD, VirtualMachine)
			RABBIT_OBJ_REF_TYPE_DECLARE(rabbit::OT_FUNCPROTO, rabbit::FunctionProto)
			
			RABBIT_SCALAR_TYPE_DECLARE(rabbit::OT_INTEGER, int64_t)
			RABBIT_SCALAR_TYPE_DECLARE(rabbit::OT_FLOAT, float_t)
			RABBIT_SCALAR_TYPE_DECLARE(rabbit::OT_USERPOINTER, UserPointer)
			
			ObjectPtr(bool _value);
			ObjectPtr& operator=(bool _value);
			~ObjectPtr();
			ObjectPtr& operator=(const ObjectPtr& _obj);
			ObjectPtr& operator=(const Object& _obj);
			void Null();
			void swap(rabbit::ObjectPtr& _obj);
		private:
			ObjectPtr(const char * _obj){} //safety
	};
	
	
	uint64_t translateIndex(const rabbit::ObjectPtr &idx);
	const char *getTypeName(const rabbit::ObjectPtr &obj1);
	const char *IdType2Name(rabbit::ObjectType type);

}
