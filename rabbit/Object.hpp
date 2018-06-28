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
	
	#define _integer(obj) ((obj)._unVal.nInteger)
	#define _float(obj) ((obj)._unVal.fFloat)
	#define _string(obj) ((obj)._unVal.pString)
	#define _table(obj) ((obj)._unVal.pTable)
	#define _array(obj) ((obj)._unVal.pArray)
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
	
	#define tofloat(num) ((sq_type(num)==rabbit::OT_INTEGER)?(float_t)_integer(num):_float(num))
	#define tointeger(num) ((sq_type(num)==rabbit::OT_FLOAT)?(int64_t)_float(num):_integer(num))
	
	
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
