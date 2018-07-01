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

#if (defined(_WIN64) || defined(_LP64))
#ifndef _SQ64
#define _SQ64
#endif
#endif

#include "sqconfig.hpp"

#define RABBIT_VERSION    _SC("Rabbit 0.1 un-stable")
#define RABBIT_COPYRIGHT  _SC("Copyright (C) 2003-2017 Alberto Demichelis")
#define RABBIT_AUTHOR     _SC("Edouard DUPIN")
#define RABBIT_VERSION_NUMBER 010

#define SQ_VMSTATE_IDLE       0
#define SQ_VMSTATE_RUNNING    1
#define SQ_VMSTATE_SUSPENDED  2

#define RABBIT_EOB 0
#define SQ_BYTECODE_STREAM_TAG  0xFAFA

#include <rabbit/Object.hpp>
#include <rabbit/Hash.hpp>




typedef int64_t (*SQFUNCTION)(rabbit::VirtualMachine*);
typedef int64_t (*SQRELEASEHOOK)(rabbit::UserPointer,int64_t size);
typedef void (*SQCOMPILERERROR)(rabbit::VirtualMachine*,const char * /*desc*/,const char * /*source*/,int64_t /*line*/,int64_t /*column*/);
typedef void (*SQPRINTFUNCTION)(rabbit::VirtualMachine*,const char * ,...);
typedef void (*SQDEBUGHOOK)(rabbit::VirtualMachine* /*v*/, int64_t /*type*/, const char * /*sourcename*/, int64_t /*line*/, const char * /*funcname*/);
typedef int64_t (*SQWRITEFUNC)(rabbit::UserPointer,rabbit::UserPointer,int64_t);
typedef int64_t (*SQREADFUNC)(rabbit::UserPointer,rabbit::UserPointer,int64_t);

typedef int64_t (*SQLEXREADFUNC)(rabbit::UserPointer);

namespace rabbit {
/*vm*/
rabbit::VirtualMachine* sq_open(int64_t initialstacksize);
rabbit::VirtualMachine* sq_newthread(rabbit::VirtualMachine* friendvm, int64_t initialstacksize);
void sq_seterrorhandler(rabbit::VirtualMachine* v);
void sq_close(rabbit::VirtualMachine* v);
void sq_setforeignptr(rabbit::VirtualMachine* v,rabbit::UserPointer p);
rabbit::UserPointer sq_getforeignptr(rabbit::VirtualMachine* v);
void sq_setsharedforeignptr(rabbit::VirtualMachine* v,rabbit::UserPointer p);
rabbit::UserPointer sq_getsharedforeignptr(rabbit::VirtualMachine* v);
void sq_setvmreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook);
SQRELEASEHOOK sq_getvmreleasehook(rabbit::VirtualMachine* v);
void sq_setsharedreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook);
SQRELEASEHOOK sq_getsharedreleasehook(rabbit::VirtualMachine* v);
void sq_setprintfunc(rabbit::VirtualMachine* v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc);
SQPRINTFUNCTION sq_getprintfunc(rabbit::VirtualMachine* v);
SQPRINTFUNCTION sq_geterrorfunc(rabbit::VirtualMachine* v);
rabbit::Result sq_suspendvm(rabbit::VirtualMachine* v);
rabbit::Result sq_wakeupvm(rabbit::VirtualMachine* v,rabbit::Bool resumedret,rabbit::Bool retval,rabbit::Bool raiseerror,rabbit::Bool throwerror);
int64_t sq_getvmstate(rabbit::VirtualMachine* v);
int64_t sq_getversion();

/*compiler*/
rabbit::Result sq_compile(rabbit::VirtualMachine* v,SQLEXREADFUNC read,rabbit::UserPointer p,const char *sourcename,rabbit::Bool raiseerror);
rabbit::Result sq_compilebuffer(rabbit::VirtualMachine* v,const char *s,int64_t size,const char *sourcename,rabbit::Bool raiseerror);
void sq_enabledebuginfo(rabbit::VirtualMachine* v, rabbit::Bool enable);
void sq_notifyallexceptions(rabbit::VirtualMachine* v, rabbit::Bool enable);
void sq_setcompilererrorhandler(rabbit::VirtualMachine* v,SQCOMPILERERROR f);

/*stack operations*/
void sq_push(rabbit::VirtualMachine* v,int64_t idx);
void sq_pop(rabbit::VirtualMachine* v,int64_t nelemstopop);
void sq_poptop(rabbit::VirtualMachine* v);
void sq_remove(rabbit::VirtualMachine* v,int64_t idx);
int64_t sq_gettop(rabbit::VirtualMachine* v);
void sq_settop(rabbit::VirtualMachine* v,int64_t newtop);
rabbit::Result sq_reservestack(rabbit::VirtualMachine* v,int64_t nsize);
int64_t sq_cmp(rabbit::VirtualMachine* v);
void sq_move(rabbit::VirtualMachine* dest,rabbit::VirtualMachine* src,int64_t idx);

/*object creation handling*/
rabbit::UserPointer sq_newuserdata(rabbit::VirtualMachine* v,uint64_t size);
void sq_newtable(rabbit::VirtualMachine* v);
void sq_newtableex(rabbit::VirtualMachine* v,int64_t initialcapacity);
void sq_newarray(rabbit::VirtualMachine* v,int64_t size);
void sq_newclosure(rabbit::VirtualMachine* v,SQFUNCTION func,uint64_t nfreevars);
rabbit::Result sq_setparamscheck(rabbit::VirtualMachine* v,int64_t nparamscheck,const char *typemask);
rabbit::Result sq_bindenv(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_setclosureroot(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_getclosureroot(rabbit::VirtualMachine* v,int64_t idx);
void sq_pushstring(rabbit::VirtualMachine* v,const char *s,int64_t len);
void sq_pushfloat(rabbit::VirtualMachine* v,float_t f);
void sq_pushinteger(rabbit::VirtualMachine* v,int64_t n);
void sq_pushbool(rabbit::VirtualMachine* v,rabbit::Bool b);
void sq_pushuserpointer(rabbit::VirtualMachine* v,rabbit::UserPointer p);
void sq_pushnull(rabbit::VirtualMachine* v);
void sq_pushthread(rabbit::VirtualMachine* v, rabbit::VirtualMachine* thread);
rabbit::ObjectType sq_gettype(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_typeof(rabbit::VirtualMachine* v,int64_t idx);
int64_t sq_getsize(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Hash sq_gethash(rabbit::VirtualMachine* v, int64_t idx);
rabbit::Result sq_getbase(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Bool sq_instanceof(rabbit::VirtualMachine* v);
rabbit::Result sq_tostring(rabbit::VirtualMachine* v,int64_t idx);
void sq_tobool(rabbit::VirtualMachine* v, int64_t idx, rabbit::Bool *b);
rabbit::Result sq_getstringandsize(rabbit::VirtualMachine* v,int64_t idx,const char **c,int64_t *size);
rabbit::Result sq_getstring(rabbit::VirtualMachine* v,int64_t idx,const char **c);
rabbit::Result sq_getinteger(rabbit::VirtualMachine* v,int64_t idx,int64_t *i);
rabbit::Result sq_getfloat(rabbit::VirtualMachine* v,int64_t idx,float_t *f);
rabbit::Result sq_getbool(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool *b);
rabbit::Result sq_getthread(rabbit::VirtualMachine* v,int64_t idx,rabbit::VirtualMachine* *thread);
rabbit::Result sq_getuserpointer(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *p);
rabbit::Result sq_getuserdata(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *p,rabbit::UserPointer *typetag);
rabbit::Result sq_settypetag(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer typetag);
rabbit::Result sq_gettypetag(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *typetag);
void sq_setreleasehook(rabbit::VirtualMachine* v,int64_t idx,SQRELEASEHOOK hook);
SQRELEASEHOOK sq_getreleasehook(rabbit::VirtualMachine* v,int64_t idx);
char *sq_getscratchpad(rabbit::VirtualMachine* v,int64_t minsize);
rabbit::Result sq_getfunctioninfo(rabbit::VirtualMachine* v,int64_t level,rabbit::FunctionInfo *fi);
rabbit::Result sq_getclosureinfo(rabbit::VirtualMachine* v,int64_t idx,uint64_t *nparams,uint64_t *nfreevars);
rabbit::Result sq_getclosurename(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_setnativeclosurename(rabbit::VirtualMachine* v,int64_t idx,const char *name);
rabbit::Result sq_setinstanceup(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer p);
rabbit::Result sq_getinstanceup(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer *p,rabbit::UserPointer typetag);
rabbit::Result sq_setclassudsize(rabbit::VirtualMachine* v, int64_t idx, int64_t udsize);
rabbit::Result sq_newclass(rabbit::VirtualMachine* v,rabbit::Bool hasbase);
rabbit::Result sq_createinstance(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_setattributes(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_getattributes(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_getclass(rabbit::VirtualMachine* v,int64_t idx);
void sq_weakref(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_getdefaultdelegate(rabbit::VirtualMachine* v,rabbit::ObjectType t);
rabbit::Result sq_getmemberhandle(rabbit::VirtualMachine* v,int64_t idx,rabbit::MemberHandle *handle);
rabbit::Result sq_getbyhandle(rabbit::VirtualMachine* v,int64_t idx,const rabbit::MemberHandle *handle);
rabbit::Result sq_setbyhandle(rabbit::VirtualMachine* v,int64_t idx,const rabbit::MemberHandle *handle);

/*object manipulation*/
void sq_pushroottable(rabbit::VirtualMachine* v);
void sq_pushregistrytable(rabbit::VirtualMachine* v);
void sq_pushconsttable(rabbit::VirtualMachine* v);
rabbit::Result sq_setroottable(rabbit::VirtualMachine* v);
rabbit::Result sq_setconsttable(rabbit::VirtualMachine* v);
rabbit::Result sq_newslot(rabbit::VirtualMachine* v, int64_t idx, rabbit::Bool bstatic);
rabbit::Result sq_deleteslot(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval);
rabbit::Result sq_set(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_get(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_rawget(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_rawset(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_rawdeleteslot(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval);
rabbit::Result sq_newmember(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool bstatic);
rabbit::Result sq_rawnewmember(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool bstatic);
rabbit::Result sq_arrayappend(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_arraypop(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval);
rabbit::Result sq_arrayresize(rabbit::VirtualMachine* v,int64_t idx,int64_t newsize);
rabbit::Result sq_arrayreverse(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_arrayremove(rabbit::VirtualMachine* v,int64_t idx,int64_t itemidx);
rabbit::Result sq_arrayinsert(rabbit::VirtualMachine* v,int64_t idx,int64_t destpos);
rabbit::Result sq_setdelegate(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_getdelegate(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_clone(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_setfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval);
rabbit::Result sq_next(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_getweakrefval(rabbit::VirtualMachine* v,int64_t idx);
rabbit::Result sq_clear(rabbit::VirtualMachine* v,int64_t idx);

/*calls*/
rabbit::Result sq_call(rabbit::VirtualMachine* v,int64_t params,rabbit::Bool retval,rabbit::Bool raiseerror);
rabbit::Result sq_resume(rabbit::VirtualMachine* v,rabbit::Bool retval,rabbit::Bool raiseerror);
const char *sq_getlocal(rabbit::VirtualMachine* v,uint64_t level,uint64_t idx);
rabbit::Result sq_getcallee(rabbit::VirtualMachine* v);
const char *sq_getfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval);
rabbit::Result sq_throwerror(rabbit::VirtualMachine* v,const char *err);
rabbit::Result sq_throwobject(rabbit::VirtualMachine* v);
void sq_reseterror(rabbit::VirtualMachine* v);
void sq_getlasterror(rabbit::VirtualMachine* v);
rabbit::Result sq_tailcall(rabbit::VirtualMachine* v, int64_t nparams);

/*raw object handling*/
rabbit::Result sq_getstackobj(rabbit::VirtualMachine* v,int64_t idx,rabbit::Object *po);
void sq_pushobject(rabbit::VirtualMachine* v,rabbit::Object obj);
void sq_addref(rabbit::VirtualMachine* v,rabbit::Object *po);
rabbit::Bool sq_release(rabbit::VirtualMachine* v,rabbit::Object *po);
uint64_t sq_getrefcount(rabbit::VirtualMachine* v,rabbit::Object *po);
void sq_resetobject(rabbit::Object *po);
const char *sq_objtostring(const rabbit::Object *o);
rabbit::Bool sq_objtobool(const rabbit::Object *o);
int64_t sq_objtointeger(const rabbit::Object *o);
float_t sq_objtofloat(const rabbit::Object *o);
rabbit::UserPointer sq_objtouserpointer(const rabbit::Object *o);
rabbit::Result sq_getobjtypetag(const rabbit::Object *o,rabbit::UserPointer * typetag);
uint64_t sq_getvmrefcount(rabbit::VirtualMachine* v, const rabbit::Object *po);


/*GC*/
int64_t sq_collectgarbage(rabbit::VirtualMachine* v);
rabbit::Result sq_resurrectunreachable(rabbit::VirtualMachine* v);

/*serialization*/
rabbit::Result sq_writeclosure(rabbit::VirtualMachine* vm,SQWRITEFUNC writef,rabbit::UserPointer up);
rabbit::Result sq_readclosure(rabbit::VirtualMachine* vm,SQREADFUNC readf,rabbit::UserPointer up);

/*mem allocation*/
void *sq_malloc(uint64_t size);
void *sq_realloc(void* p,uint64_t oldsize,uint64_t newsize);
void sq_free(void *p,uint64_t size);

/*debug*/
rabbit::Result sq_stackinfos(rabbit::VirtualMachine* v,int64_t level,rabbit::StackInfos *si);
void sq_setdebughook(rabbit::VirtualMachine* v);
void sq_setnativedebughook(rabbit::VirtualMachine* v,SQDEBUGHOOK hook);

}

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

