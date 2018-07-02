/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/ObjectPtr.hpp>
#include <rabbit/RefCounted.hpp>

#define RABBIT_OBJ_REF_TYPE_INSTANCIATE(type,_class,sym) \
	rabbit::ObjectPtr::ObjectPtr(_class * x) \
	{ \
		_unVal.raw = 0; \
		_type=type; \
		_unVal.sym = x; \
		assert(_unVal.pTable); \
		_unVal.pRefCounted->refCountIncrement(); \
	} \
	rabbit::ObjectPtr& rabbit::ObjectPtr::operator=(_class *x) \
	{  \
		rabbit::ObjectType tOldType; \
		rabbit::ObjectValue unOldVal; \
		tOldType=_type; \
		unOldVal=_unVal; \
		_type = type; \
		_unVal.raw = 0; \
		_unVal.sym = x; \
		_unVal.pRefCounted->refCountIncrement(); \
		__release(tOldType,unOldVal); \
		return *this; \
	}

#define RABBIT_SCALAR_TYPE_INSTANCIATE(type,_class,sym) \
	rabbit::ObjectPtr::ObjectPtr(_class x) \
	{ \
		_unVal.raw = 0; \
		_type=type; \
		_unVal.sym = x; \
	} \
	rabbit::ObjectPtr& rabbit::ObjectPtr::operator=(_class x) \
	{  \
		__release(_type,_unVal); \
		_type = type; \
		_unVal.raw = 0; \
		_unVal.sym = x; \
		return *this; \
	}


RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_TABLE, rabbit::Table, pTable)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_CLASS, rabbit::Class, pClass)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_INSTANCE, rabbit::Instance, pInstance)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_ARRAY, rabbit::Array, pArray)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_CLOSURE, rabbit::Closure, pClosure)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_NATIVECLOSURE, rabbit::NativeClosure, pNativeClosure)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_OUTER, rabbit::Outer, pOuter)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_GENERATOR, rabbit::Generator, pGenerator)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_STRING, rabbit::String, pString)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_USERDATA, rabbit::UserData, pUserData)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_WEAKREF, rabbit::WeakRef, pWeakRef)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_THREAD, rabbit::VirtualMachine, pThread)
RABBIT_OBJ_REF_TYPE_INSTANCIATE(rabbit::OT_FUNCPROTO, rabbit::FunctionProto, pFunctionProto)

RABBIT_SCALAR_TYPE_INSTANCIATE(rabbit::OT_INTEGER, int64_t, nInteger)
RABBIT_SCALAR_TYPE_INSTANCIATE(rabbit::OT_FLOAT, float_t, fFloat)
RABBIT_SCALAR_TYPE_INSTANCIATE(rabbit::OT_USERPOINTER, rabbit::UserPointer, pUserPointer)



rabbit::ObjectPtr::ObjectPtr() {
	_unVal.raw = 0;
	_type = rabbit::OT_NULL;
	_unVal.pUserPointer = NULL;
}

rabbit::ObjectPtr::ObjectPtr(const rabbit::ObjectPtr& _obj) {
	_type = _obj._type;
	_unVal = _obj._unVal;
	__addRef(_type,_unVal);
}

rabbit::ObjectPtr::ObjectPtr(const rabbit::Object& _obj) {
	_type = _obj._type;
	_unVal = _obj._unVal;
	__addRef(_type,_unVal);
}

rabbit::ObjectPtr::ObjectPtr(bool _value) {
	_unVal.raw = 0;
	_type = rabbit::OT_BOOL;
	_unVal.nInteger = _value?1:0;
}

rabbit::ObjectPtr& rabbit::ObjectPtr::operator=(bool _value) {
	__release(_type,_unVal);
	_unVal.raw = 0;
	_type = rabbit::OT_BOOL;
	_unVal.nInteger = _value?1:0;
	return *this;
}

rabbit::ObjectPtr::~ObjectPtr() {
	__release(_type,_unVal);
}

rabbit::ObjectPtr& rabbit::ObjectPtr::operator=(const rabbit::ObjectPtr& _obj) {
	rabbit::ObjectType tOldType;
	rabbit::ObjectValue unOldVal;
	tOldType=_type;
	unOldVal=_unVal;
	_unVal = _obj._unVal;
	_type = _obj._type;
	__addRef(_type,_unVal);
	__release(tOldType,unOldVal);
	return *this;
}

rabbit::ObjectPtr& rabbit::ObjectPtr::operator=(const rabbit::Object& _obj) {
	rabbit::ObjectType tOldType;
	rabbit::ObjectValue unOldVal;
	tOldType=_type;
	unOldVal=_unVal;
	_unVal = _obj._unVal;
	_type = _obj._type;
	__addRef(_type,_unVal);
	__release(tOldType,unOldVal);
	return *this;
}

void rabbit::ObjectPtr::Null() {
	rabbit::ObjectType tOldType = _type;
	rabbit::ObjectValue unOldVal = _unVal;
	_type = rabbit::OT_NULL;
	_unVal.raw = 0;
	__release(tOldType ,unOldVal);
}



uint64_t rabbit::translateIndex(const rabbit::ObjectPtr &idx)
{
	switch(sq_type(idx)){
		case rabbit::OT_NULL:
			return 0;
		case rabbit::OT_INTEGER:
			return (uint64_t)_integer(idx);
		default: assert(0); break;
	}
	return 0;
}


const char* rabbit::IdType2Name(rabbit::ObjectType type)
{
	switch(_RAW_TYPE(type))
	{
		case _RT_NULL:
			return "null";
		case _RT_INTEGER:
			return "integer";
		case _RT_FLOAT:
			return "float";
		case _RT_BOOL:
			return "bool";
		case _RT_STRING:
			return "string";
		case _RT_TABLE:
			return "table";
		case _RT_ARRAY:
			return "array";
		case _RT_GENERATOR:
			return "generator";
		case _RT_CLOSURE:
		case _RT_NATIVECLOSURE:
			return "function";
		case _RT_USERDATA:
		case _RT_USERPOINTER:
			return "userdata";
		case _RT_THREAD:
			return "thread";
		case _RT_FUNCPROTO:
			return "function";
		case _RT_CLASS:
			return "class";
		case _RT_INSTANCE:
			return "instance";
		case _RT_WEAKREF:
			return "weakref";
		case _RT_OUTER:
			return "outer";
		default:
			return NULL;
	}
}

const char* rabbit::getTypeName(const rabbit::ObjectPtr &obj1)
{
	return IdType2Name(sq_type(obj1));
}
