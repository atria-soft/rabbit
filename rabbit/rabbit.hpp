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

#ifdef __cplusplus
extern "C" {
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

struct SQVM;
struct SQTable;
struct SQArray;
struct SQString;
struct SQClosure;
struct SQGenerator;
struct SQNativeClosure;
struct SQUserData;
struct SQFunctionProto;
struct SQRefCounted;
struct SQClass;
struct SQInstance;
struct SQDelegable;
struct SQOuter;

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
	OT_NULL =		   (_RT_NULL|SQOBJECT_CANBEFALSE),
	OT_INTEGER =		(_RT_INTEGER|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_FLOAT =		  (_RT_FLOAT|SQOBJECT_NUMERIC|SQOBJECT_CANBEFALSE),
	OT_BOOL =		   (_RT_BOOL|SQOBJECT_CANBEFALSE),
	OT_STRING =		 (_RT_STRING|SQOBJECT_REF_COUNTED),
	OT_TABLE =		  (_RT_TABLE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_ARRAY =		  (_RT_ARRAY|SQOBJECT_REF_COUNTED),
	OT_USERDATA =	   (_RT_USERDATA|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_CLOSURE =		(_RT_CLOSURE|SQOBJECT_REF_COUNTED),
	OT_NATIVECLOSURE =  (_RT_NATIVECLOSURE|SQOBJECT_REF_COUNTED),
	OT_GENERATOR =	  (_RT_GENERATOR|SQOBJECT_REF_COUNTED),
	OT_USERPOINTER =	_RT_USERPOINTER,
	OT_THREAD =		 (_RT_THREAD|SQOBJECT_REF_COUNTED) ,
	OT_FUNCPROTO =	  (_RT_FUNCPROTO|SQOBJECT_REF_COUNTED), //internal usage only
	OT_CLASS =		  (_RT_CLASS|SQOBJECT_REF_COUNTED),
	OT_INSTANCE =	   (_RT_INSTANCE|SQOBJECT_REF_COUNTED|SQOBJECT_DELEGABLE),
	OT_WEAKREF =		(_RT_WEAKREF|SQOBJECT_REF_COUNTED),
	OT_OUTER =		  (_RT_OUTER|SQOBJECT_REF_COUNTED) //internal usage only
}SQObjectType;

#define ISREFCOUNTED(t) (t&SQOBJECT_REF_COUNTED)


typedef union tagSQObjectValue
{
	struct SQTable *pTable;
	struct SQArray *pArray;
	struct SQClosure *pClosure;
	struct SQOuter *pOuter;
	struct SQGenerator *pGenerator;
	struct SQNativeClosure *pNativeClosure;
	struct SQString *pString;
	struct SQUserData *pUserData;
	SQInteger nInteger;
	SQFloat fFloat;
	SQUserPointer pUserPointer;
	struct SQFunctionProto *pFunctionProto;
	struct SQRefCounted *pRefCounted;
	struct SQDelegable *pDelegable;
	struct SQVM *pThread;
	struct SQClass *pClass;
	struct SQInstance *pInstance;
	struct SQWeakRef *pWeakRef;
	SQRawObjectVal raw;
}SQObjectValue;


typedef struct tagSQObject
{
	SQObjectType _type;
	SQObjectValue _unVal;
}SQObject;

typedef struct  tagSQMemberHandle{
	SQBool _static;
	SQInteger _index;
}SQMemberHandle;

typedef struct tagSQStackInfos{
	const SQChar* funcname;
	const SQChar* source;
	SQInteger line;
}SQStackInfos;

typedef struct SQVM* HRABBITVM;
typedef SQObject HSQOBJECT;
typedef SQMemberHandle HSQMEMBERHANDLE;
typedef SQInteger (*SQFUNCTION)(HRABBITVM);
typedef SQInteger (*SQRELEASEHOOK)(SQUserPointer,SQInteger size);
typedef void (*SQCOMPILERERROR)(HRABBITVM,const SQChar * /*desc*/,const SQChar * /*source*/,SQInteger /*line*/,SQInteger /*column*/);
typedef void (*SQPRINTFUNCTION)(HRABBITVM,const SQChar * ,...);
typedef void (*SQDEBUGHOOK)(HRABBITVM /*v*/, SQInteger /*type*/, const SQChar * /*sourcename*/, SQInteger /*line*/, const SQChar * /*funcname*/);
typedef SQInteger (*SQWRITEFUNC)(SQUserPointer,SQUserPointer,SQInteger);
typedef SQInteger (*SQREADFUNC)(SQUserPointer,SQUserPointer,SQInteger);

typedef SQInteger (*SQLEXREADFUNC)(SQUserPointer);

typedef struct tagSQRegFunction{
	const SQChar *name;
	SQFUNCTION f;
	SQInteger nparamscheck;
	const SQChar *typemask;
}SQRegFunction;

typedef struct tagSQFunctionInfo {
	SQUserPointer funcid;
	const SQChar *name;
	const SQChar *source;
	SQInteger line;
}SQFunctionInfo;

/*vm*/
RABBIT_API HRABBITVM sq_open(SQInteger initialstacksize);
RABBIT_API HRABBITVM sq_newthread(HRABBITVM friendvm, SQInteger initialstacksize);
RABBIT_API void sq_seterrorhandler(HRABBITVM v);
RABBIT_API void sq_close(HRABBITVM v);
RABBIT_API void sq_setforeignptr(HRABBITVM v,SQUserPointer p);
RABBIT_API SQUserPointer sq_getforeignptr(HRABBITVM v);
RABBIT_API void sq_setsharedforeignptr(HRABBITVM v,SQUserPointer p);
RABBIT_API SQUserPointer sq_getsharedforeignptr(HRABBITVM v);
RABBIT_API void sq_setvmreleasehook(HRABBITVM v,SQRELEASEHOOK hook);
RABBIT_API SQRELEASEHOOK sq_getvmreleasehook(HRABBITVM v);
RABBIT_API void sq_setsharedreleasehook(HRABBITVM v,SQRELEASEHOOK hook);
RABBIT_API SQRELEASEHOOK sq_getsharedreleasehook(HRABBITVM v);
RABBIT_API void sq_setprintfunc(HRABBITVM v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc);
RABBIT_API SQPRINTFUNCTION sq_getprintfunc(HRABBITVM v);
RABBIT_API SQPRINTFUNCTION sq_geterrorfunc(HRABBITVM v);
RABBIT_API SQRESULT sq_suspendvm(HRABBITVM v);
RABBIT_API SQRESULT sq_wakeupvm(HRABBITVM v,SQBool resumedret,SQBool retval,SQBool raiseerror,SQBool throwerror);
RABBIT_API SQInteger sq_getvmstate(HRABBITVM v);
RABBIT_API SQInteger sq_getversion();

/*compiler*/
RABBIT_API SQRESULT sq_compile(HRABBITVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,SQBool raiseerror);
RABBIT_API SQRESULT sq_compilebuffer(HRABBITVM v,const SQChar *s,SQInteger size,const SQChar *sourcename,SQBool raiseerror);
RABBIT_API void sq_enabledebuginfo(HRABBITVM v, SQBool enable);
RABBIT_API void sq_notifyallexceptions(HRABBITVM v, SQBool enable);
RABBIT_API void sq_setcompilererrorhandler(HRABBITVM v,SQCOMPILERERROR f);

/*stack operations*/
RABBIT_API void sq_push(HRABBITVM v,SQInteger idx);
RABBIT_API void sq_pop(HRABBITVM v,SQInteger nelemstopop);
RABBIT_API void sq_poptop(HRABBITVM v);
RABBIT_API void sq_remove(HRABBITVM v,SQInteger idx);
RABBIT_API SQInteger sq_gettop(HRABBITVM v);
RABBIT_API void sq_settop(HRABBITVM v,SQInteger newtop);
RABBIT_API SQRESULT sq_reservestack(HRABBITVM v,SQInteger nsize);
RABBIT_API SQInteger sq_cmp(HRABBITVM v);
RABBIT_API void sq_move(HRABBITVM dest,HRABBITVM src,SQInteger idx);

/*object creation handling*/
RABBIT_API SQUserPointer sq_newuserdata(HRABBITVM v,SQUnsignedInteger size);
RABBIT_API void sq_newtable(HRABBITVM v);
RABBIT_API void sq_newtableex(HRABBITVM v,SQInteger initialcapacity);
RABBIT_API void sq_newarray(HRABBITVM v,SQInteger size);
RABBIT_API void sq_newclosure(HRABBITVM v,SQFUNCTION func,SQUnsignedInteger nfreevars);
RABBIT_API SQRESULT sq_setparamscheck(HRABBITVM v,SQInteger nparamscheck,const SQChar *typemask);
RABBIT_API SQRESULT sq_bindenv(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_setclosureroot(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_getclosureroot(HRABBITVM v,SQInteger idx);
RABBIT_API void sq_pushstring(HRABBITVM v,const SQChar *s,SQInteger len);
RABBIT_API void sq_pushfloat(HRABBITVM v,SQFloat f);
RABBIT_API void sq_pushinteger(HRABBITVM v,SQInteger n);
RABBIT_API void sq_pushbool(HRABBITVM v,SQBool b);
RABBIT_API void sq_pushuserpointer(HRABBITVM v,SQUserPointer p);
RABBIT_API void sq_pushnull(HRABBITVM v);
RABBIT_API void sq_pushthread(HRABBITVM v, HRABBITVM thread);
RABBIT_API SQObjectType sq_gettype(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_typeof(HRABBITVM v,SQInteger idx);
RABBIT_API SQInteger sq_getsize(HRABBITVM v,SQInteger idx);
RABBIT_API SQHash sq_gethash(HRABBITVM v, SQInteger idx);
RABBIT_API SQRESULT sq_getbase(HRABBITVM v,SQInteger idx);
RABBIT_API SQBool sq_instanceof(HRABBITVM v);
RABBIT_API SQRESULT sq_tostring(HRABBITVM v,SQInteger idx);
RABBIT_API void sq_tobool(HRABBITVM v, SQInteger idx, SQBool *b);
RABBIT_API SQRESULT sq_getstringandsize(HRABBITVM v,SQInteger idx,const SQChar **c,SQInteger *size);
RABBIT_API SQRESULT sq_getstring(HRABBITVM v,SQInteger idx,const SQChar **c);
RABBIT_API SQRESULT sq_getinteger(HRABBITVM v,SQInteger idx,SQInteger *i);
RABBIT_API SQRESULT sq_getfloat(HRABBITVM v,SQInteger idx,SQFloat *f);
RABBIT_API SQRESULT sq_getbool(HRABBITVM v,SQInteger idx,SQBool *b);
RABBIT_API SQRESULT sq_getthread(HRABBITVM v,SQInteger idx,HRABBITVM *thread);
RABBIT_API SQRESULT sq_getuserpointer(HRABBITVM v,SQInteger idx,SQUserPointer *p);
RABBIT_API SQRESULT sq_getuserdata(HRABBITVM v,SQInteger idx,SQUserPointer *p,SQUserPointer *typetag);
RABBIT_API SQRESULT sq_settypetag(HRABBITVM v,SQInteger idx,SQUserPointer typetag);
RABBIT_API SQRESULT sq_gettypetag(HRABBITVM v,SQInteger idx,SQUserPointer *typetag);
RABBIT_API void sq_setreleasehook(HRABBITVM v,SQInteger idx,SQRELEASEHOOK hook);
RABBIT_API SQRELEASEHOOK sq_getreleasehook(HRABBITVM v,SQInteger idx);
RABBIT_API SQChar *sq_getscratchpad(HRABBITVM v,SQInteger minsize);
RABBIT_API SQRESULT sq_getfunctioninfo(HRABBITVM v,SQInteger level,SQFunctionInfo *fi);
RABBIT_API SQRESULT sq_getclosureinfo(HRABBITVM v,SQInteger idx,SQUnsignedInteger *nparams,SQUnsignedInteger *nfreevars);
RABBIT_API SQRESULT sq_getclosurename(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_setnativeclosurename(HRABBITVM v,SQInteger idx,const SQChar *name);
RABBIT_API SQRESULT sq_setinstanceup(HRABBITVM v, SQInteger idx, SQUserPointer p);
RABBIT_API SQRESULT sq_getinstanceup(HRABBITVM v, SQInteger idx, SQUserPointer *p,SQUserPointer typetag);
RABBIT_API SQRESULT sq_setclassudsize(HRABBITVM v, SQInteger idx, SQInteger udsize);
RABBIT_API SQRESULT sq_newclass(HRABBITVM v,SQBool hasbase);
RABBIT_API SQRESULT sq_createinstance(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_setattributes(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_getattributes(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_getclass(HRABBITVM v,SQInteger idx);
RABBIT_API void sq_weakref(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_getdefaultdelegate(HRABBITVM v,SQObjectType t);
RABBIT_API SQRESULT sq_getmemberhandle(HRABBITVM v,SQInteger idx,HSQMEMBERHANDLE *handle);
RABBIT_API SQRESULT sq_getbyhandle(HRABBITVM v,SQInteger idx,const HSQMEMBERHANDLE *handle);
RABBIT_API SQRESULT sq_setbyhandle(HRABBITVM v,SQInteger idx,const HSQMEMBERHANDLE *handle);

/*object manipulation*/
RABBIT_API void sq_pushroottable(HRABBITVM v);
RABBIT_API void sq_pushregistrytable(HRABBITVM v);
RABBIT_API void sq_pushconsttable(HRABBITVM v);
RABBIT_API SQRESULT sq_setroottable(HRABBITVM v);
RABBIT_API SQRESULT sq_setconsttable(HRABBITVM v);
RABBIT_API SQRESULT sq_newslot(HRABBITVM v, SQInteger idx, SQBool bstatic);
RABBIT_API SQRESULT sq_deleteslot(HRABBITVM v,SQInteger idx,SQBool pushval);
RABBIT_API SQRESULT sq_set(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_get(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_rawget(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_rawset(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_rawdeleteslot(HRABBITVM v,SQInteger idx,SQBool pushval);
RABBIT_API SQRESULT sq_newmember(HRABBITVM v,SQInteger idx,SQBool bstatic);
RABBIT_API SQRESULT sq_rawnewmember(HRABBITVM v,SQInteger idx,SQBool bstatic);
RABBIT_API SQRESULT sq_arrayappend(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_arraypop(HRABBITVM v,SQInteger idx,SQBool pushval);
RABBIT_API SQRESULT sq_arrayresize(HRABBITVM v,SQInteger idx,SQInteger newsize);
RABBIT_API SQRESULT sq_arrayreverse(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_arrayremove(HRABBITVM v,SQInteger idx,SQInteger itemidx);
RABBIT_API SQRESULT sq_arrayinsert(HRABBITVM v,SQInteger idx,SQInteger destpos);
RABBIT_API SQRESULT sq_setdelegate(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_getdelegate(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_clone(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_setfreevariable(HRABBITVM v,SQInteger idx,SQUnsignedInteger nval);
RABBIT_API SQRESULT sq_next(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_getweakrefval(HRABBITVM v,SQInteger idx);
RABBIT_API SQRESULT sq_clear(HRABBITVM v,SQInteger idx);

/*calls*/
RABBIT_API SQRESULT sq_call(HRABBITVM v,SQInteger params,SQBool retval,SQBool raiseerror);
RABBIT_API SQRESULT sq_resume(HRABBITVM v,SQBool retval,SQBool raiseerror);
RABBIT_API const SQChar *sq_getlocal(HRABBITVM v,SQUnsignedInteger level,SQUnsignedInteger idx);
RABBIT_API SQRESULT sq_getcallee(HRABBITVM v);
RABBIT_API const SQChar *sq_getfreevariable(HRABBITVM v,SQInteger idx,SQUnsignedInteger nval);
RABBIT_API SQRESULT sq_throwerror(HRABBITVM v,const SQChar *err);
RABBIT_API SQRESULT sq_throwobject(HRABBITVM v);
RABBIT_API void sq_reseterror(HRABBITVM v);
RABBIT_API void sq_getlasterror(HRABBITVM v);
RABBIT_API SQRESULT sq_tailcall(HRABBITVM v, SQInteger nparams);

/*raw object handling*/
RABBIT_API SQRESULT sq_getstackobj(HRABBITVM v,SQInteger idx,HSQOBJECT *po);
RABBIT_API void sq_pushobject(HRABBITVM v,HSQOBJECT obj);
RABBIT_API void sq_addref(HRABBITVM v,HSQOBJECT *po);
RABBIT_API SQBool sq_release(HRABBITVM v,HSQOBJECT *po);
RABBIT_API SQUnsignedInteger sq_getrefcount(HRABBITVM v,HSQOBJECT *po);
RABBIT_API void sq_resetobject(HSQOBJECT *po);
RABBIT_API const SQChar *sq_objtostring(const HSQOBJECT *o);
RABBIT_API SQBool sq_objtobool(const HSQOBJECT *o);
RABBIT_API SQInteger sq_objtointeger(const HSQOBJECT *o);
RABBIT_API SQFloat sq_objtofloat(const HSQOBJECT *o);
RABBIT_API SQUserPointer sq_objtouserpointer(const HSQOBJECT *o);
RABBIT_API SQRESULT sq_getobjtypetag(const HSQOBJECT *o,SQUserPointer * typetag);
RABBIT_API SQUnsignedInteger sq_getvmrefcount(HRABBITVM v, const HSQOBJECT *po);


/*GC*/
RABBIT_API SQInteger sq_collectgarbage(HRABBITVM v);
RABBIT_API SQRESULT sq_resurrectunreachable(HRABBITVM v);

/*serialization*/
RABBIT_API SQRESULT sq_writeclosure(HRABBITVM vm,SQWRITEFUNC writef,SQUserPointer up);
RABBIT_API SQRESULT sq_readclosure(HRABBITVM vm,SQREADFUNC readf,SQUserPointer up);

/*mem allocation*/
RABBIT_API void *sq_malloc(SQUnsignedInteger size);
RABBIT_API void *sq_realloc(void* p,SQUnsignedInteger oldsize,SQUnsignedInteger newsize);
RABBIT_API void sq_free(void *p,SQUnsignedInteger size);

/*debug*/
RABBIT_API SQRESULT sq_stackinfos(HRABBITVM v,SQInteger level,SQStackInfos *si);
RABBIT_API void sq_setdebughook(HRABBITVM v);
RABBIT_API void sq_setnativedebughook(HRABBITVM v,SQDEBUGHOOK hook);

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

#ifdef __cplusplus
} /*extern "C"*/
#endif

