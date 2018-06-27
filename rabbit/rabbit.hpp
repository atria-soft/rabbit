/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#ifdef _SQ_CONFIG_INCLUDE
#include _SQ_CONFIG_INCLUDE
#endif

#ifndef RABBIT_API
#define RABBIT_API extern
#endif

#if (defined(_WIN64) || defined(_LP64))
#ifndef _SQ64
#define _SQ64
#endif
#endif


#define SQTrue  (1)
#define SQFalse (0)

struct SQTable;
struct SQString;
struct SQClosure;
struct SQGenerator;
struct SQNativeClosure;
struct SQFunctionProto;
struct SQClass;
struct SQInstance;
struct SQDelegable;
struct SQOuter;
namespace rabbit {
	class UserData;
	class Array;
	class RefCounted;
	class WeakRef;
	class VirtualMachine;
	
}
#ifdef _UNICODE
#define SQUNICODE
#endif

#include "sqconfig.hpp"

#define RABBIT_VERSION	_SC("Rabbit 0.1 un-stable")
#define RABBIT_COPYRIGHT  _SC("Copyright (C) 2003-2017 Alberto Demichelis")
#define RABBIT_AUTHOR	 _SC("Edouard DUPIN")
#define RABBIT_VERSION_NUMBER 010

#define SQ_VMSTATE_IDLE		 0
#define SQ_VMSTATE_RUNNING	  1
#define SQ_VMSTATE_SUSPENDED	2

#define RABBIT_EOB 0
#define SQ_BYTECODE_STREAM_TAG  0xFAFA

#define SQOBJECT_REF_COUNTED	0x08000000
#define SQOBJECT_NUMERIC		0x04000000
#define SQOBJECT_DELEGABLE	  0x02000000
#define SQOBJECT_CANBEFALSE	 0x01000000

#define SQ_MATCHTYPEMASKSTRING (-99999)

#define _RT_MASK 0x00FFFFFF
#define _RAW_TYPE(type) (type&_RT_MASK)

#define _RT_NULL			0x00000001
#define _RT_INTEGER		 0x00000002
#define _RT_FLOAT		   0x00000004
#define _RT_BOOL			0x00000008
#define _RT_STRING		  0x00000010
#define _RT_TABLE		   0x00000020
#define _RT_ARRAY		   0x00000040
#define _RT_USERDATA		0x00000080
#define _RT_CLOSURE		 0x00000100
#define _RT_NATIVECLOSURE   0x00000200
#define _RT_GENERATOR	   0x00000400
#define _RT_USERPOINTER	 0x00000800
#define _RT_THREAD		  0x00001000
#define _RT_FUNCPROTO	   0x00002000
#define _RT_CLASS		   0x00004000
#define _RT_INSTANCE		0x00008000
#define _RT_WEAKREF		 0x00010000
#define _RT_OUTER		   0x00020000

typedef enum tagSQObjectType{
	OT_NULL =           (_RT_NULL|SQOBJECT_CANBEFALSE),
	OT_INTEGER =        (_RT_INTEGER|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_FLOAT =          (_RT_FLOAT|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_BOOL =           (_RT_BOOL|SQOBJECT_CANBEFALSE),
	OT_STRING =         (_RT_STRING|SQOBJECT_REF_COUNTED),
	OT_TABLE =          (_RT_TABLE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_ARRAY =          (_RT_ARRAY|SQOBJECT_REF_COUNTED),
	OT_USERDATA =       (_RT_USERDATA|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_CLOSURE =        (_RT_CLOSURE|SQOBJECT_REF_COUNTED),
	OT_NATIVECLOSURE =  (_RT_NATIVECLOSURE|SQOBJECT_REF_COUNTED),
	OT_GENERATOR =      (_RT_GENERATOR|SQOBJECT_REF_COUNTED),
	OT_USERPOINTER =    _RT_USERPOINTER,
	OT_THREAD =         (_RT_THREAD|SQOBJECT_REF_COUNTED) ,
	OT_FUNCPROTO =      (_RT_FUNCPROTO|SQOBJECT_REF_COUNTED), //internal usage only
	OT_CLASS =          (_RT_CLASS|SQOBJECT_REF_COUNTED),
	OT_INSTANCE =       (_RT_INSTANCE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_WEAKREF =        (_RT_WEAKREF|SQOBJECT_REF_COUNTED),
	OT_OUTER =          (_RT_OUTER|SQOBJECT_REF_COUNTED) //internal usage only
}SQObjectType;

#define ISREFCOUNTED(t) (t&SQOBJECT_REF_COUNTED)


typedef union tagSQObjectValue
{
	struct SQTable *pTable;
	struct SQClosure *pClosure;
	struct SQOuter *pOuter;
	struct SQGenerator *pGenerator;
	struct SQNativeClosure *pNativeClosure;
	struct SQString *pString;
	int64_t nInteger;
	float_t fFloat;
	SQUserPointer pUserPointer;
	struct SQFunctionProto *pFunctionProto;
	struct SQDelegable *pDelegable;
	struct SQClass *pClass;
	struct SQInstance *pInstance;
	
	struct rabbit::WeakRef *pWeakRef;
	struct rabbit::VirtualMachine* pThread;
	struct rabbit::RefCounted *pRefCounted;
	struct rabbit::Array *pArray;
	struct rabbit::UserData *pUserData;
	
	SQRawObjectVal raw;
}SQObjectValue;


typedef struct tagSQObject
{
	SQObjectType _type;
	SQObjectValue _unVal;
}SQObject;

typedef struct tagSQMemberHandle{
	SQBool _static;
	int64_t _index;
}SQMemberHandle;

typedef struct tagSQStackInfos{
	const SQChar* funcname;
	const SQChar* source;
	int64_t line;
}SQStackInfos;

typedef SQObject HSQOBJECT;
typedef SQMemberHandle HSQMEMBERHANDLE;
typedef int64_t (*SQFUNCTION)(rabbit::VirtualMachine*);
typedef int64_t (*SQRELEASEHOOK)(SQUserPointer,int64_t size);
typedef void (*SQCOMPILERERROR)(rabbit::VirtualMachine*,const SQChar * /*desc*/,const SQChar * /*source*/,int64_t /*line*/,int64_t /*column*/);
typedef void (*SQPRINTFUNCTION)(rabbit::VirtualMachine*,const SQChar * ,...);
typedef void (*SQDEBUGHOOK)(rabbit::VirtualMachine* /*v*/, int64_t /*type*/, const SQChar * /*sourcename*/, int64_t /*line*/, const SQChar * /*funcname*/);
typedef int64_t (*SQWRITEFUNC)(SQUserPointer,SQUserPointer,int64_t);
typedef int64_t (*SQREADFUNC)(SQUserPointer,SQUserPointer,int64_t);

typedef int64_t (*SQLEXREADFUNC)(SQUserPointer);

typedef struct tagSQRegFunction{
	const SQChar *name;
	SQFUNCTION f;
	int64_t nparamscheck;
	const SQChar *typemask;
}SQRegFunction;

typedef struct tagSQFunctionInfo {
	SQUserPointer funcid;
	const SQChar *name;
	const SQChar *source;
	int64_t line;
}SQFunctionInfo;

/*vm*/
RABBIT_API rabbit::VirtualMachine* sq_open(int64_t initialstacksize);
RABBIT_API rabbit::VirtualMachine* sq_newthread(rabbit::VirtualMachine* friendvm, int64_t initialstacksize);
RABBIT_API void sq_seterrorhandler(rabbit::VirtualMachine* v);
RABBIT_API void sq_close(rabbit::VirtualMachine* v);
RABBIT_API void sq_setforeignptr(rabbit::VirtualMachine* v,SQUserPointer p);
RABBIT_API SQUserPointer sq_getforeignptr(rabbit::VirtualMachine* v);
RABBIT_API void sq_setsharedforeignptr(rabbit::VirtualMachine* v,SQUserPointer p);
RABBIT_API SQUserPointer sq_getsharedforeignptr(rabbit::VirtualMachine* v);
RABBIT_API void sq_setvmreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook);
RABBIT_API SQRELEASEHOOK sq_getvmreleasehook(rabbit::VirtualMachine* v);
RABBIT_API void sq_setsharedreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook);
RABBIT_API SQRELEASEHOOK sq_getsharedreleasehook(rabbit::VirtualMachine* v);
RABBIT_API void sq_setprintfunc(rabbit::VirtualMachine* v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc);
RABBIT_API SQPRINTFUNCTION sq_getprintfunc(rabbit::VirtualMachine* v);
RABBIT_API SQPRINTFUNCTION sq_geterrorfunc(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_suspendvm(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_wakeupvm(rabbit::VirtualMachine* v,SQBool resumedret,SQBool retval,SQBool raiseerror,SQBool throwerror);
RABBIT_API int64_t sq_getvmstate(rabbit::VirtualMachine* v);
RABBIT_API int64_t sq_getversion();

/*compiler*/
RABBIT_API SQRESULT sq_compile(rabbit::VirtualMachine* v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,SQBool raiseerror);
RABBIT_API SQRESULT sq_compilebuffer(rabbit::VirtualMachine* v,const SQChar *s,int64_t size,const SQChar *sourcename,SQBool raiseerror);
RABBIT_API void sq_enabledebuginfo(rabbit::VirtualMachine* v, SQBool enable);
RABBIT_API void sq_notifyallexceptions(rabbit::VirtualMachine* v, SQBool enable);
RABBIT_API void sq_setcompilererrorhandler(rabbit::VirtualMachine* v,SQCOMPILERERROR f);

/*stack operations*/
RABBIT_API void sq_push(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API void sq_pop(rabbit::VirtualMachine* v,int64_t nelemstopop);
RABBIT_API void sq_poptop(rabbit::VirtualMachine* v);
RABBIT_API void sq_remove(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API int64_t sq_gettop(rabbit::VirtualMachine* v);
RABBIT_API void sq_settop(rabbit::VirtualMachine* v,int64_t newtop);
RABBIT_API SQRESULT sq_reservestack(rabbit::VirtualMachine* v,int64_t nsize);
RABBIT_API int64_t sq_cmp(rabbit::VirtualMachine* v);
RABBIT_API void sq_move(rabbit::VirtualMachine* dest,rabbit::VirtualMachine* src,int64_t idx);

/*object creation handling*/
RABBIT_API SQUserPointer sq_newuserdata(rabbit::VirtualMachine* v,uint64_t size);
RABBIT_API void sq_newtable(rabbit::VirtualMachine* v);
RABBIT_API void sq_newtableex(rabbit::VirtualMachine* v,int64_t initialcapacity);
RABBIT_API void sq_newarray(rabbit::VirtualMachine* v,int64_t size);
RABBIT_API void sq_newclosure(rabbit::VirtualMachine* v,SQFUNCTION func,uint64_t nfreevars);
RABBIT_API SQRESULT sq_setparamscheck(rabbit::VirtualMachine* v,int64_t nparamscheck,const SQChar *typemask);
RABBIT_API SQRESULT sq_bindenv(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_setclosureroot(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_getclosureroot(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API void sq_pushstring(rabbit::VirtualMachine* v,const SQChar *s,int64_t len);
RABBIT_API void sq_pushfloat(rabbit::VirtualMachine* v,float_t f);
RABBIT_API void sq_pushinteger(rabbit::VirtualMachine* v,int64_t n);
RABBIT_API void sq_pushbool(rabbit::VirtualMachine* v,SQBool b);
RABBIT_API void sq_pushuserpointer(rabbit::VirtualMachine* v,SQUserPointer p);
RABBIT_API void sq_pushnull(rabbit::VirtualMachine* v);
RABBIT_API void sq_pushthread(rabbit::VirtualMachine* v, rabbit::VirtualMachine* thread);
RABBIT_API SQObjectType sq_gettype(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_typeof(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API int64_t sq_getsize(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQHash sq_gethash(rabbit::VirtualMachine* v, int64_t idx);
RABBIT_API SQRESULT sq_getbase(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQBool sq_instanceof(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_tostring(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API void sq_tobool(rabbit::VirtualMachine* v, int64_t idx, SQBool *b);
RABBIT_API SQRESULT sq_getstringandsize(rabbit::VirtualMachine* v,int64_t idx,const SQChar **c,int64_t *size);
RABBIT_API SQRESULT sq_getstring(rabbit::VirtualMachine* v,int64_t idx,const SQChar **c);
RABBIT_API SQRESULT sq_getinteger(rabbit::VirtualMachine* v,int64_t idx,int64_t *i);
RABBIT_API SQRESULT sq_getfloat(rabbit::VirtualMachine* v,int64_t idx,float_t *f);
RABBIT_API SQRESULT sq_getbool(rabbit::VirtualMachine* v,int64_t idx,SQBool *b);
RABBIT_API SQRESULT sq_getthread(rabbit::VirtualMachine* v,int64_t idx,rabbit::VirtualMachine* *thread);
RABBIT_API SQRESULT sq_getuserpointer(rabbit::VirtualMachine* v,int64_t idx,SQUserPointer *p);
RABBIT_API SQRESULT sq_getuserdata(rabbit::VirtualMachine* v,int64_t idx,SQUserPointer *p,SQUserPointer *typetag);
RABBIT_API SQRESULT sq_settypetag(rabbit::VirtualMachine* v,int64_t idx,SQUserPointer typetag);
RABBIT_API SQRESULT sq_gettypetag(rabbit::VirtualMachine* v,int64_t idx,SQUserPointer *typetag);
RABBIT_API void sq_setreleasehook(rabbit::VirtualMachine* v,int64_t idx,SQRELEASEHOOK hook);
RABBIT_API SQRELEASEHOOK sq_getreleasehook(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQChar *sq_getscratchpad(rabbit::VirtualMachine* v,int64_t minsize);
RABBIT_API SQRESULT sq_getfunctioninfo(rabbit::VirtualMachine* v,int64_t level,SQFunctionInfo *fi);
RABBIT_API SQRESULT sq_getclosureinfo(rabbit::VirtualMachine* v,int64_t idx,uint64_t *nparams,uint64_t *nfreevars);
RABBIT_API SQRESULT sq_getclosurename(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_setnativeclosurename(rabbit::VirtualMachine* v,int64_t idx,const SQChar *name);
RABBIT_API SQRESULT sq_setinstanceup(rabbit::VirtualMachine* v, int64_t idx, SQUserPointer p);
RABBIT_API SQRESULT sq_getinstanceup(rabbit::VirtualMachine* v, int64_t idx, SQUserPointer *p,SQUserPointer typetag);
RABBIT_API SQRESULT sq_setclassudsize(rabbit::VirtualMachine* v, int64_t idx, int64_t udsize);
RABBIT_API SQRESULT sq_newclass(rabbit::VirtualMachine* v,SQBool hasbase);
RABBIT_API SQRESULT sq_createinstance(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_setattributes(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_getattributes(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_getclass(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API void sq_weakref(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_getdefaultdelegate(rabbit::VirtualMachine* v,SQObjectType t);
RABBIT_API SQRESULT sq_getmemberhandle(rabbit::VirtualMachine* v,int64_t idx,HSQMEMBERHANDLE *handle);
RABBIT_API SQRESULT sq_getbyhandle(rabbit::VirtualMachine* v,int64_t idx,const HSQMEMBERHANDLE *handle);
RABBIT_API SQRESULT sq_setbyhandle(rabbit::VirtualMachine* v,int64_t idx,const HSQMEMBERHANDLE *handle);

/*object manipulation*/
RABBIT_API void sq_pushroottable(rabbit::VirtualMachine* v);
RABBIT_API void sq_pushregistrytable(rabbit::VirtualMachine* v);
RABBIT_API void sq_pushconsttable(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_setroottable(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_setconsttable(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_newslot(rabbit::VirtualMachine* v, int64_t idx, SQBool bstatic);
RABBIT_API SQRESULT sq_deleteslot(rabbit::VirtualMachine* v,int64_t idx,SQBool pushval);
RABBIT_API SQRESULT sq_set(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_get(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_rawget(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_rawset(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_rawdeleteslot(rabbit::VirtualMachine* v,int64_t idx,SQBool pushval);
RABBIT_API SQRESULT sq_newmember(rabbit::VirtualMachine* v,int64_t idx,SQBool bstatic);
RABBIT_API SQRESULT sq_rawnewmember(rabbit::VirtualMachine* v,int64_t idx,SQBool bstatic);
RABBIT_API SQRESULT sq_arrayappend(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_arraypop(rabbit::VirtualMachine* v,int64_t idx,SQBool pushval);
RABBIT_API SQRESULT sq_arrayresize(rabbit::VirtualMachine* v,int64_t idx,int64_t newsize);
RABBIT_API SQRESULT sq_arrayreverse(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_arrayremove(rabbit::VirtualMachine* v,int64_t idx,int64_t itemidx);
RABBIT_API SQRESULT sq_arrayinsert(rabbit::VirtualMachine* v,int64_t idx,int64_t destpos);
RABBIT_API SQRESULT sq_setdelegate(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_getdelegate(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_clone(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_setfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval);
RABBIT_API SQRESULT sq_next(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_getweakrefval(rabbit::VirtualMachine* v,int64_t idx);
RABBIT_API SQRESULT sq_clear(rabbit::VirtualMachine* v,int64_t idx);

/*calls*/
RABBIT_API SQRESULT sq_call(rabbit::VirtualMachine* v,int64_t params,SQBool retval,SQBool raiseerror);
RABBIT_API SQRESULT sq_resume(rabbit::VirtualMachine* v,SQBool retval,SQBool raiseerror);
RABBIT_API const SQChar *sq_getlocal(rabbit::VirtualMachine* v,uint64_t level,uint64_t idx);
RABBIT_API SQRESULT sq_getcallee(rabbit::VirtualMachine* v);
RABBIT_API const SQChar *sq_getfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval);
RABBIT_API SQRESULT sq_throwerror(rabbit::VirtualMachine* v,const SQChar *err);
RABBIT_API SQRESULT sq_throwobject(rabbit::VirtualMachine* v);
RABBIT_API void sq_reseterror(rabbit::VirtualMachine* v);
RABBIT_API void sq_getlasterror(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_tailcall(rabbit::VirtualMachine* v, int64_t nparams);

/*raw object handling*/
RABBIT_API SQRESULT sq_getstackobj(rabbit::VirtualMachine* v,int64_t idx,HSQOBJECT *po);
RABBIT_API void sq_pushobject(rabbit::VirtualMachine* v,HSQOBJECT obj);
RABBIT_API void sq_addref(rabbit::VirtualMachine* v,HSQOBJECT *po);
RABBIT_API SQBool sq_release(rabbit::VirtualMachine* v,HSQOBJECT *po);
RABBIT_API uint64_t sq_getrefcount(rabbit::VirtualMachine* v,HSQOBJECT *po);
RABBIT_API void sq_resetobject(HSQOBJECT *po);
RABBIT_API const SQChar *sq_objtostring(const HSQOBJECT *o);
RABBIT_API SQBool sq_objtobool(const HSQOBJECT *o);
RABBIT_API int64_t sq_objtointeger(const HSQOBJECT *o);
RABBIT_API float_t sq_objtofloat(const HSQOBJECT *o);
RABBIT_API SQUserPointer sq_objtouserpointer(const HSQOBJECT *o);
RABBIT_API SQRESULT sq_getobjtypetag(const HSQOBJECT *o,SQUserPointer * typetag);
RABBIT_API uint64_t sq_getvmrefcount(rabbit::VirtualMachine* v, const HSQOBJECT *po);


/*GC*/
RABBIT_API int64_t sq_collectgarbage(rabbit::VirtualMachine* v);
RABBIT_API SQRESULT sq_resurrectunreachable(rabbit::VirtualMachine* v);

/*serialization*/
RABBIT_API SQRESULT sq_writeclosure(rabbit::VirtualMachine* vm,SQWRITEFUNC writef,SQUserPointer up);
RABBIT_API SQRESULT sq_readclosure(rabbit::VirtualMachine* vm,SQREADFUNC readf,SQUserPointer up);

/*mem allocation*/
RABBIT_API void *sq_malloc(uint64_t size);
RABBIT_API void *sq_realloc(void* p,uint64_t oldsize,uint64_t newsize);
RABBIT_API void sq_free(void *p,uint64_t size);

/*debug*/
RABBIT_API SQRESULT sq_stackinfos(rabbit::VirtualMachine* v,int64_t level,SQStackInfos *si);
RABBIT_API void sq_setdebughook(rabbit::VirtualMachine* v);
RABBIT_API void sq_setnativedebughook(rabbit::VirtualMachine* v,SQDEBUGHOOK hook);

/*UTILITY MACRO*/
#define sq_isnumeric(o) ((o)._type&SQOBJECT_NUMERIC)
#define sq_istable(o) ((o)._type==OT_TABLE)
#define sq_isarray(o) ((o)._type==OT_ARRAY)
#define sq_isfunction(o) ((o)._type==OT_FUNCPROTO)
#define sq_isclosure(o) ((o)._type==OT_CLOSURE)
#define sq_isgenerator(o) ((o)._type==OT_GENERATOR)
#define sq_isnativeclosure(o) ((o)._type==OT_NATIVECLOSURE)
#define sq_isstring(o) ((o)._type==OT_STRING)
#define sq_isinteger(o) ((o)._type==OT_INTEGER)
#define sq_isfloat(o) ((o)._type==OT_FLOAT)
#define sq_isuserpointer(o) ((o)._type==OT_USERPOINTER)
#define sq_isuserdata(o) ((o)._type==OT_USERDATA)
#define sq_isthread(o) ((o)._type==OT_THREAD)
#define sq_isnull(o) ((o)._type==OT_NULL)
#define sq_isclass(o) ((o)._type==OT_CLASS)
#define sq_isinstance(o) ((o)._type==OT_INSTANCE)
#define sq_isbool(o) ((o)._type==OT_BOOL)
#define sq_isweakref(o) ((o)._type==OT_WEAKREF)
#define sq_type(o) ((o)._type)

/* deprecated */
#define sq_createslot(v,n) sq_newslot(v,n,SQFalse)

#define SQ_OK (0)
#define SQ_ERROR (-1)

#define SQ_FAILED(res) (res<0)
#define SQ_SUCCEEDED(res) (res>=0)

#ifdef __GNUC__
# define SQ_UNUSED_ARG(x) x __attribute__((__unused__))
#else
# define SQ_UNUSED_ARG(x) x
#endif

