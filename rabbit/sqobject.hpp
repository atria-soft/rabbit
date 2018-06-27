/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/squtils.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/WeakRef.hpp>

#ifdef _SQ64
#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)
#else
#define UINT_MINUS_ONE (0xFFFFFFFF)
#endif

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

struct SQSharedState;

enum SQMetaMethod{
	MT_ADD=0,
	MT_SUB=1,
	MT_MUL=2,
	MT_DIV=3,
	MT_UNM=4,
	MT_MODULO=5,
	MT_SET=6,
	MT_GET=7,
	MT_TYPEOF=8,
	MT_NEXTI=9,
	MT_CMP=10,
	MT_CALL=11,
	MT_CLONED=12,
	MT_NEWSLOT=13,
	MT_DELSLOT=14,
	MT_TOSTRING=15,
	MT_NEWMEMBER=16,
	MT_INHERITED=17,
	MT_LAST = 18
};

#define MM_ADD	  _SC("_add")
#define MM_SUB	  _SC("_sub")
#define MM_MUL	  _SC("_mul")
#define MM_DIV	  _SC("_div")
#define MM_UNM	  _SC("_unm")
#define MM_MODULO   _SC("_modulo")
#define MM_SET	  _SC("_set")
#define MM_GET	  _SC("_get")
#define MM_TYPEOF   _SC("_typeof")
#define MM_NEXTI	_SC("_nexti")
#define MM_CMP	  _SC("_cmp")
#define MM_CALL	 _SC("_call")
#define MM_CLONED   _SC("_cloned")
#define MM_NEWSLOT  _SC("_newslot")
#define MM_DELSLOT  _SC("_delslot")
#define MM_TOSTRING _SC("_tostring")
#define MM_NEWMEMBER _SC("_newmember")
#define MM_INHERITED _SC("_inherited")


#define _CONSTRUCT_VECTOR(type,size,ptr) { \
	for(int64_t n = 0; n < ((int64_t)size); n++) { \
			new (&ptr[n]) type(); \
		} \
}

#define _DESTRUCT_VECTOR(type,size,ptr) { \
	for(int64_t nl = 0; nl < ((int64_t)size); nl++) { \
			ptr[nl].~type(); \
	} \
}

#define _COPY_VECTOR(dest,src,size) { \
	for(int64_t _n_ = 0; _n_ < ((int64_t)size); _n_++) { \
		dest[_n_] = src[_n_]; \
	} \
}

#define _NULL_SQOBJECT_VECTOR(vec,size) { \
	for(int64_t _n_ = 0; _n_ < ((int64_t)size); _n_++) { \
		vec[_n_].Null(); \
	} \
}

#define MINPOWER2 4


#define _realval(o) (sq_type((o)) != OT_WEAKREF?(SQObject)o:_weakref(o)->_obj)

struct SQObjectPtr;

#define __addRef(type,unval) if(ISREFCOUNTED(type)) \
		{ \
			unval.pRefCounted->refCountIncrement(); \
		}

#define __release(type,unval) if(ISREFCOUNTED(type) && (unval.pRefCounted->refCountDecrement()==0))  \
		{   \
			unval.pRefCounted->release();   \
		}

#define __Objrelease(obj) { \
	if((obj)) { \
		auto val = (obj)->refCountDecrement(); \
		if(val == 0) { \
			(obj)->release(); \
		} \
		(obj) = NULL;   \
	} \
}

#define __ObjaddRef(obj) { \
	(obj)->refCountIncrement(); \
}

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
#define _delegable(obj) ((SQDelegable *)(obj)._unVal.pDelegable)
#define _weakref(obj) ((obj)._unVal.pWeakRef)
#define _outer(obj) ((obj)._unVal.pOuter)
#define _refcounted(obj) ((obj)._unVal.pRefCounted)
#define _rawval(obj) ((obj)._unVal.raw)

#define _stringval(obj) (obj)._unVal.pString->_val
#define _userdataval(obj) ((SQUserPointer)sq_aligning((obj)._unVal.pUserData + 1))

#define tofloat(num) ((sq_type(num)==OT_INTEGER)?(float_t)_integer(num):_float(num))
#define tointeger(num) ((sq_type(num)==OT_FLOAT)?(int64_t)_float(num):_integer(num))
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
#define SQ_REFOBJECT_INIT() SQ_OBJECT_RAWINIT()
#else
#define SQ_REFOBJECT_INIT()
#endif

#define _REF_TYPE_DECL(type,_class,sym) \
	SQObjectPtr(_class * x) \
	{ \
		SQ_OBJECT_RAWINIT() \
		_type=type; \
		_unVal.sym = x; \
		assert(_unVal.pTable); \
		_unVal.pRefCounted->refCountIncrement(); \
	} \
	inline SQObjectPtr& operator=(_class *x) \
	{  \
		SQObjectType tOldType; \
		SQObjectValue unOldVal; \
		tOldType=_type; \
		unOldVal=_unVal; \
		_type = type; \
		SQ_REFOBJECT_INIT() \
		_unVal.sym = x; \
		_unVal.pRefCounted->refCountIncrement(); \
		__release(tOldType,unOldVal); \
		return *this; \
	}

#define _SCALAR_TYPE_DECL(type,_class,sym) \
	SQObjectPtr(_class x) \
	{ \
		SQ_OBJECT_RAWINIT() \
		_type=type; \
		_unVal.sym = x; \
	} \
	inline SQObjectPtr& operator=(_class x) \
	{  \
		__release(_type,_unVal); \
		_type = type; \
		SQ_OBJECT_RAWINIT() \
		_unVal.sym = x; \
		return *this; \
	}
struct SQObjectPtr : public SQObject
{
	SQObjectPtr()
	{
		SQ_OBJECT_RAWINIT()
		_type=OT_NULL;
		_unVal.pUserPointer=NULL;
	}
	SQObjectPtr(const SQObjectPtr &o)
	{
		_type = o._type;
		_unVal = o._unVal;
		__addRef(_type,_unVal);
	}
	SQObjectPtr(const SQObject &o)
	{
		_type = o._type;
		_unVal = o._unVal;
		__addRef(_type,_unVal);
	}
	_REF_TYPE_DECL(OT_TABLE,SQTable,pTable)
	_REF_TYPE_DECL(OT_CLASS,SQClass,pClass)
	_REF_TYPE_DECL(OT_INSTANCE,SQInstance,pInstance)
	_REF_TYPE_DECL(OT_ARRAY,rabbit::Array,pArray)
	_REF_TYPE_DECL(OT_CLOSURE,SQClosure,pClosure)
	_REF_TYPE_DECL(OT_NATIVECLOSURE,SQNativeClosure,pNativeClosure)
	_REF_TYPE_DECL(OT_OUTER,SQOuter,pOuter)
	_REF_TYPE_DECL(OT_GENERATOR,SQGenerator,pGenerator)
	_REF_TYPE_DECL(OT_STRING,SQString,pString)
	_REF_TYPE_DECL(OT_USERDATA,rabbit::UserData,pUserData)
	_REF_TYPE_DECL(OT_WEAKREF,rabbit::WeakRef,pWeakRef)
	_REF_TYPE_DECL(OT_THREAD,rabbit::VirtualMachine,pThread)
	_REF_TYPE_DECL(OT_FUNCPROTO,SQFunctionProto,pFunctionProto)

	_SCALAR_TYPE_DECL(OT_INTEGER,int64_t,nInteger)
	_SCALAR_TYPE_DECL(OT_FLOAT,float_t,fFloat)
	_SCALAR_TYPE_DECL(OT_USERPOINTER,SQUserPointer,pUserPointer)

	SQObjectPtr(bool bBool)
	{
		SQ_OBJECT_RAWINIT()
		_type = OT_BOOL;
		_unVal.nInteger = bBool?1:0;
	}
	inline SQObjectPtr& operator=(bool b)
	{
		__release(_type,_unVal);
		SQ_OBJECT_RAWINIT()
		_type = OT_BOOL;
		_unVal.nInteger = b?1:0;
		return *this;
	}

	~SQObjectPtr()
	{
		__release(_type,_unVal);
	}

	inline SQObjectPtr& operator=(const SQObjectPtr& obj)
	{
		SQObjectType tOldType;
		SQObjectValue unOldVal;
		tOldType=_type;
		unOldVal=_unVal;
		_unVal = obj._unVal;
		_type = obj._type;
		__addRef(_type,_unVal);
		__release(tOldType,unOldVal);
		return *this;
	}
	inline SQObjectPtr& operator=(const SQObject& obj)
	{
		SQObjectType tOldType;
		SQObjectValue unOldVal;
		tOldType=_type;
		unOldVal=_unVal;
		_unVal = obj._unVal;
		_type = obj._type;
		__addRef(_type,_unVal);
		__release(tOldType,unOldVal);
		return *this;
	}
	inline void Null()
	{
		SQObjectType tOldType = _type;
		SQObjectValue unOldVal = _unVal;
		_type = OT_NULL;
		_unVal.raw = (SQRawObjectVal)NULL;
		__release(tOldType ,unOldVal);
	}
	private:
		SQObjectPtr(const SQChar *){} //safety
};


inline void _Swap(SQObject &a,SQObject &b)
{
	SQObjectType tOldType = a._type;
	SQObjectValue unOldVal = a._unVal;
	a._type = b._type;
	a._unVal = b._unVal;
	b._type = tOldType;
	b._unVal = unOldVal;
}


struct SQDelegable : public rabbit::RefCounted {
	bool setDelegate(SQTable *m);
	virtual bool getMetaMethod(rabbit::VirtualMachine *v,SQMetaMethod mm,SQObjectPtr &res);
	SQTable *_delegate;
};

uint64_t translateIndex(const SQObjectPtr &idx);
typedef etk::Vector<SQObjectPtr> SQObjectPtrVec;
typedef etk::Vector<int64_t> SQIntVec;
const SQChar *getTypeName(const SQObjectPtr &obj1);
const SQChar *IdType2Name(SQObjectType type);



