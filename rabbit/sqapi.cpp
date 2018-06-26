/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/sqpcheader.hpp>
#include <rabbit/sqvm.hpp>
#include <rabbit/sqstring.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/Array.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/UserData.hpp>
#include <rabbit/sqcompiler.hpp>
#include <rabbit/sqfuncstate.hpp>
#include <rabbit/sqclass.hpp>

static bool sq_aux_gettypedarg(HRABBITVM v,int64_t idx,SQObjectType type,SQObjectPtr **o)
{
	*o = &stack_get(v,idx);
	if(sq_type(**o) != type){
		SQObjectPtr oval = v->PrintObjVal(**o);
		v->Raise_Error(_SC("wrong argument type, expected '%s' got '%.50s'"),IdType2Name(type),_stringval(oval));
		return false;
	}
	return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { if(!sq_aux_gettypedarg(v,idx,type,&o)) return SQ_ERROR; }

#define sq_aux_paramscheck(v,count) \
{ \
	if(sq_gettop(v) < count){ v->Raise_Error(_SC("not enough params in the stack")); return SQ_ERROR; }\
}


int64_t sq_aux_invalidtype(HRABBITVM v,SQObjectType type)
{
	uint64_t buf_size = 100 *sizeof(SQChar);
	scsprintf(_ss(v)->getScratchPad(buf_size), buf_size, _SC("unexpected type %s"), IdType2Name(type));
	return sq_throwerror(v, _ss(v)->getScratchPad(-1));
}

HRABBITVM sq_open(int64_t initialstacksize)
{
	SQSharedState *ss;
	SQVM *v;
	sq_new(ss, SQSharedState);
	ss->Init();
	v = (SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);
	ss->_root_vm = v;
	if(v->Init(NULL, initialstacksize)) {
		return v;
	} else {
		sq_delete(v, SQVM);
		return NULL;
	}
	return v;
}

HRABBITVM sq_newthread(HRABBITVM friendvm, int64_t initialstacksize)
{
	SQSharedState *ss;
	SQVM *v;
	ss=_ss(friendvm);

	v= (SQVM *)SQ_MALLOC(sizeof(SQVM));
	new (v) SQVM(ss);

	if(v->Init(friendvm, initialstacksize)) {
		friendvm->Push(v);
		return v;
	} else {
		sq_delete(v, SQVM);
		return NULL;
	}
}

int64_t sq_getvmstate(HRABBITVM v)
{
	if(v->_suspended)
		return SQ_VMSTATE_SUSPENDED;
	else {
		if(v->_callsstacksize != 0) return SQ_VMSTATE_RUNNING;
		else return SQ_VMSTATE_IDLE;
	}
}

void sq_seterrorhandler(HRABBITVM v)
{
	SQObject o = stack_get(v, -1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
		v->_errorhandler = o;
		v->Pop();
	}
}

void sq_setnativedebughook(HRABBITVM v,SQDEBUGHOOK hook)
{
	v->_debughook_native = hook;
	v->_debughook_closure.Null();
	v->_debughook = hook?true:false;
}

void sq_setdebughook(HRABBITVM v)
{
	SQObject o = stack_get(v,-1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
		v->_debughook_closure = o;
		v->_debughook_native = NULL;
		v->_debughook = !sq_isnull(o);
		v->Pop();
	}
}

void sq_close(HRABBITVM v)
{
	SQSharedState *ss = _ss(v);
	_thread(ss->_root_vm)->Finalize();
	sq_delete(ss, SQSharedState);
}

int64_t sq_getversion()
{
	return RABBIT_VERSION_NUMBER;
}

SQRESULT sq_compile(HRABBITVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,SQBool raiseerror)
{
	SQObjectPtr o;
#ifndef NO_COMPILER
	if(Compile(v, read, p, sourcename, o, raiseerror?true:false, _ss(v)->_debuginfo)) {
		v->Push(SQClosure::create(_ss(v), _funcproto(o), _table(v->_roottable)->getWeakRef(OT_TABLE)));
		return SQ_OK;
	}
	return SQ_ERROR;
#else
	return sq_throwerror(v,_SC("this is a no compiler build"));
#endif
}

void sq_enabledebuginfo(HRABBITVM v, SQBool enable)
{
	_ss(v)->_debuginfo = enable?true:false;
}

void sq_notifyallexceptions(HRABBITVM v, SQBool enable)
{
	_ss(v)->_notifyallexceptions = enable?true:false;
}

void sq_addref(HRABBITVM v,HSQOBJECT *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return;
	__AddRef(po->_type,po->_unVal);
}

uint64_t sq_getrefcount(HRABBITVM v,HSQOBJECT *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return 0;
   return po->_unVal.pRefCounted->refCountget();
}

SQBool sq_release(HRABBITVM v,HSQOBJECT *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return SQTrue;
	bool ret = (po->_unVal.pRefCounted->refCountget() <= 1) ? SQTrue : SQFalse;
	__release(po->_type,po->_unVal);
	return ret; //the ret val doesn't work(and cannot be fixed)
}

uint64_t sq_getvmrefcount(HRABBITVM SQ_UNUSED_ARG(v), const HSQOBJECT *po)
{
	if (!ISREFCOUNTED(sq_type(*po))) return 0;
	return po->_unVal.pRefCounted->refCountget();
}

const SQChar *sq_objtostring(const HSQOBJECT *o)
{
	if(sq_type(*o) == OT_STRING) {
		return _stringval(*o);
	}
	return NULL;
}

int64_t sq_objtointeger(const HSQOBJECT *o)
{
	if(sq_isnumeric(*o)) {
		return tointeger(*o);
	}
	return 0;
}

float_t sq_objtofloat(const HSQOBJECT *o)
{
	if(sq_isnumeric(*o)) {
		return tofloat(*o);
	}
	return 0;
}

SQBool sq_objtobool(const HSQOBJECT *o)
{
	if(sq_isbool(*o)) {
		return _integer(*o);
	}
	return SQFalse;
}

SQUserPointer sq_objtouserpointer(const HSQOBJECT *o)
{
	if(sq_isuserpointer(*o)) {
		return _userpointer(*o);
	}
	return 0;
}

void sq_pushnull(HRABBITVM v)
{
	v->PushNull();
}

void sq_pushstring(HRABBITVM v,const SQChar *s,int64_t len)
{
	if(s)
		v->Push(SQObjectPtr(SQString::create(_ss(v), s, len)));
	else v->PushNull();
}

void sq_pushinteger(HRABBITVM v,int64_t n)
{
	v->Push(n);
}

void sq_pushbool(HRABBITVM v,SQBool b)
{
	v->Push(b?true:false);
}

void sq_pushfloat(HRABBITVM v,float_t n)
{
	v->Push(n);
}

void sq_pushuserpointer(HRABBITVM v,SQUserPointer p)
{
	v->Push(p);
}

void sq_pushthread(HRABBITVM v, HRABBITVM thread)
{
	v->Push(thread);
}

SQUserPointer sq_newuserdata(HRABBITVM v,uint64_t size)
{
	rabbit::UserData *ud = rabbit::UserData::create(_ss(v), size + SQ_ALIGNMENT);
	v->Push(ud);
	return (SQUserPointer)sq_aligning(ud + 1);
}

void sq_newtable(HRABBITVM v)
{
	v->Push(SQTable::create(_ss(v), 0));
}

void sq_newtableex(HRABBITVM v,int64_t initialcapacity)
{
	v->Push(SQTable::create(_ss(v), initialcapacity));
}

void sq_newarray(HRABBITVM v,int64_t size)
{
	v->Push(rabbit::Array::create(_ss(v), size));
}

SQRESULT sq_newclass(HRABBITVM v,SQBool hasbase)
{
	SQClass *baseclass = NULL;
	if(hasbase) {
		SQObjectPtr &base = stack_get(v,-1);
		if(sq_type(base) != OT_CLASS)
			return sq_throwerror(v,_SC("invalid base type"));
		baseclass = _class(base);
	}
	SQClass *newclass = SQClass::create(_ss(v), baseclass);
	if(baseclass) v->Pop();
	v->Push(newclass);
	return SQ_OK;
}

SQBool sq_instanceof(HRABBITVM v)
{
	SQObjectPtr &inst = stack_get(v,-1);
	SQObjectPtr &cl = stack_get(v,-2);
	if(sq_type(inst) != OT_INSTANCE || sq_type(cl) != OT_CLASS)
		return sq_throwerror(v,_SC("invalid param type"));
	return _instance(inst)->InstanceOf(_class(cl))?SQTrue:SQFalse;
}

SQRESULT sq_arrayappend(HRABBITVM v,int64_t idx)
{
	sq_aux_paramscheck(v,2);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
	_array(*arr)->append(v->getUp(-1));
	v->Pop();
	return SQ_OK;
}

SQRESULT sq_arraypop(HRABBITVM v,int64_t idx,SQBool pushval)
{
	sq_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
	if(_array(*arr)->size() > 0) {
		if(pushval != 0){
			v->Push(_array(*arr)->top());
		}
		_array(*arr)->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("empty array"));
}

SQRESULT sq_arrayresize(HRABBITVM v,int64_t idx,int64_t newsize)
{
	sq_aux_paramscheck(v,1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
	if(newsize >= 0) {
		_array(*arr)->resize(newsize);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("negative size"));
}


SQRESULT sq_arrayreverse(HRABBITVM v,int64_t idx)
{
	sq_aux_paramscheck(v, 1);
	SQObjectPtr *o;
	_GETSAFE_OBJ(v, idx, OT_ARRAY,o);
	rabbit::Array *arr = _array(*o);
	if(arr->size() > 0) {
		SQObjectPtr t;
		int64_t size = arr->size();
		int64_t n = size >> 1; size -= 1;
		for(int64_t i = 0; i < n; i++) {
			// TODO: set a swap
			t = (*arr)[i];
			(*arr)[i] = (*arr)[size-i];
			(*arr)[size-i] = t;
		}
		return SQ_OK;
	}
	return SQ_OK;
}

SQRESULT sq_arrayremove(HRABBITVM v,int64_t idx,int64_t itemidx)
{
	sq_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
	return _array(*arr)->remove(itemidx) ? SQ_OK : sq_throwerror(v,_SC("index out of range"));
}

SQRESULT sq_arrayinsert(HRABBITVM v,int64_t idx,int64_t destpos)
{
	sq_aux_paramscheck(v, 1);
	SQObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, OT_ARRAY,arr);
	SQRESULT ret = _array(*arr)->insert(destpos, v->getUp(-1)) ? SQ_OK : sq_throwerror(v,_SC("index out of range"));
	v->Pop();
	return ret;
}

void sq_newclosure(HRABBITVM v,SQFUNCTION func,uint64_t nfreevars)
{
	SQNativeClosure *nc = SQNativeClosure::create(_ss(v), func,nfreevars);
	nc->_nparamscheck = 0;
	for(uint64_t i = 0; i < nfreevars; i++) {
		nc->_outervalues[i] = v->top();
		v->Pop();
	}
	v->Push(SQObjectPtr(nc));
}

SQRESULT sq_getclosureinfo(HRABBITVM v,int64_t idx,uint64_t *nparams,uint64_t *nfreevars)
{
	SQObject o = stack_get(v, idx);
	if(sq_type(o) == OT_CLOSURE) {
		SQClosure *c = _closure(o);
		SQFunctionProto *proto = c->_function;
		*nparams = (uint64_t)proto->_nparameters;
		*nfreevars = (uint64_t)proto->_noutervalues;
		return SQ_OK;
	}
	else if(sq_type(o) == OT_NATIVECLOSURE)
	{
		SQNativeClosure *c = _nativeclosure(o);
		*nparams = (uint64_t)c->_nparamscheck;
		*nfreevars = c->_noutervalues;
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("the object is not a closure"));
}

SQRESULT sq_setnativeclosurename(HRABBITVM v,int64_t idx,const SQChar *name)
{
	SQObject o = stack_get(v, idx);
	if(sq_isnativeclosure(o)) {
		SQNativeClosure *nc = _nativeclosure(o);
		nc->_name = SQString::create(_ss(v),name);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("the object is not a nativeclosure"));
}

SQRESULT sq_setparamscheck(HRABBITVM v,int64_t nparamscheck,const SQChar *typemask)
{
	SQObject o = stack_get(v, -1);
	if(!sq_isnativeclosure(o))
		return sq_throwerror(v, _SC("native closure expected"));
	SQNativeClosure *nc = _nativeclosure(o);
	nc->_nparamscheck = nparamscheck;
	if(typemask) {
		SQIntVec res;
		if(!CompileTypemask(res, typemask))
			return sq_throwerror(v, _SC("invalid typemask"));
		nc->_typecheck.copy(res);
	}
	else {
		nc->_typecheck.resize(0);
	}
	if(nparamscheck == SQ_MATCHTYPEMASKSTRING) {
		nc->_nparamscheck = nc->_typecheck.size();
	}
	return SQ_OK;
}

SQRESULT sq_bindenv(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &o = stack_get(v,idx);
	if(!sq_isnativeclosure(o) &&
		!sq_isclosure(o))
		return sq_throwerror(v,_SC("the target is not a closure"));
	SQObjectPtr &env = stack_get(v,-1);
	if(!sq_istable(env) &&
		!sq_isarray(env) &&
		!sq_isclass(env) &&
		!sq_isinstance(env))
		return sq_throwerror(v,_SC("invalid environment"));
	rabbit::WeakRef *w = _refcounted(env)->getWeakRef(sq_type(env));
	SQObjectPtr ret;
	if(sq_isclosure(o)) {
		SQClosure *c = _closure(o)->clone();
		__Objrelease(c->_env);
		c->_env = w;
		__ObjAddRef(c->_env);
		if(_closure(o)->_base) {
			c->_base = _closure(o)->_base;
			__ObjAddRef(c->_base);
		}
		ret = c;
	}
	else { //then must be a native closure
		SQNativeClosure *c = _nativeclosure(o)->clone();
		__Objrelease(c->_env);
		c->_env = w;
		__ObjAddRef(c->_env);
		ret = c;
	}
	v->Pop();
	v->Push(ret);
	return SQ_OK;
}

SQRESULT sq_getclosurename(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &o = stack_get(v,idx);
	if(!sq_isnativeclosure(o) &&
		!sq_isclosure(o))
		return sq_throwerror(v,_SC("the target is not a closure"));
	if(sq_isnativeclosure(o))
	{
		v->Push(_nativeclosure(o)->_name);
	}
	else { //closure
		v->Push(_closure(o)->_function->_name);
	}
	return SQ_OK;
}

SQRESULT sq_setclosureroot(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &c = stack_get(v,idx);
	SQObject o = stack_get(v, -1);
	if(!sq_isclosure(c)) return sq_throwerror(v, _SC("closure expected"));
	if(sq_istable(o)) {
		_closure(c)->setRoot(_table(o)->getWeakRef(OT_TABLE));
		v->Pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("invalid type"));
}

SQRESULT sq_getclosureroot(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &c = stack_get(v,idx);
	if(!sq_isclosure(c)) return sq_throwerror(v, _SC("closure expected"));
	v->Push(_closure(c)->_root->_obj);
	return SQ_OK;
}

SQRESULT sq_clear(HRABBITVM v,int64_t idx)
{
	SQObject &o=stack_get(v,idx);
	switch(sq_type(o)) {
		case OT_TABLE: _table(o)->Clear();  break;
		case OT_ARRAY: _array(o)->resize(0); break;
		default:
			return sq_throwerror(v, _SC("clear only works on table and array"));
		break;

	}
	return SQ_OK;
}

void sq_pushroottable(HRABBITVM v)
{
	v->Push(v->_roottable);
}

void sq_pushregistrytable(HRABBITVM v)
{
	v->Push(_ss(v)->_registry);
}

void sq_pushconsttable(HRABBITVM v)
{
	v->Push(_ss(v)->_consts);
}

SQRESULT sq_setroottable(HRABBITVM v)
{
	SQObject o = stack_get(v, -1);
	if(sq_istable(o) || sq_isnull(o)) {
		v->_roottable = o;
		v->Pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("invalid type"));
}

SQRESULT sq_setconsttable(HRABBITVM v)
{
	SQObject o = stack_get(v, -1);
	if(sq_istable(o)) {
		_ss(v)->_consts = o;
		v->Pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("invalid type, expected table"));
}

void sq_setforeignptr(HRABBITVM v,SQUserPointer p)
{
	v->_foreignptr = p;
}

SQUserPointer sq_getforeignptr(HRABBITVM v)
{
	return v->_foreignptr;
}

void sq_setsharedforeignptr(HRABBITVM v,SQUserPointer p)
{
	_ss(v)->_foreignptr = p;
}

SQUserPointer sq_getsharedforeignptr(HRABBITVM v)
{
	return _ss(v)->_foreignptr;
}

void sq_setvmreleasehook(HRABBITVM v,SQRELEASEHOOK hook)
{
	v->_releasehook = hook;
}

SQRELEASEHOOK sq_getvmreleasehook(HRABBITVM v)
{
	return v->_releasehook;
}

void sq_setsharedreleasehook(HRABBITVM v,SQRELEASEHOOK hook)
{
	_ss(v)->_releasehook = hook;
}

SQRELEASEHOOK sq_getsharedreleasehook(HRABBITVM v)
{
	return _ss(v)->_releasehook;
}

void sq_push(HRABBITVM v,int64_t idx)
{
	v->Push(stack_get(v, idx));
}

SQObjectType sq_gettype(HRABBITVM v,int64_t idx)
{
	return sq_type(stack_get(v, idx));
}

SQRESULT sq_typeof(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &o = stack_get(v, idx);
	SQObjectPtr res;
	if(!v->TypeOf(o,res)) {
		return SQ_ERROR;
	}
	v->Push(res);
	return SQ_OK;
}

SQRESULT sq_tostring(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &o = stack_get(v, idx);
	SQObjectPtr res;
	if(!v->ToString(o,res)) {
		return SQ_ERROR;
	}
	v->Push(res);
	return SQ_OK;
}

void sq_tobool(HRABBITVM v, int64_t idx, SQBool *b)
{
	SQObjectPtr &o = stack_get(v, idx);
	*b = SQVM::IsFalse(o)?SQFalse:SQTrue;
}

SQRESULT sq_getinteger(HRABBITVM v,int64_t idx,int64_t *i)
{
	SQObjectPtr &o = stack_get(v, idx);
	if(sq_isnumeric(o)) {
		*i = tointeger(o);
		return SQ_OK;
	}
	if(sq_isbool(o)) {
		*i = SQVM::IsFalse(o)?SQFalse:SQTrue;
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getfloat(HRABBITVM v,int64_t idx,float_t *f)
{
	SQObjectPtr &o = stack_get(v, idx);
	if(sq_isnumeric(o)) {
		*f = tofloat(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getbool(HRABBITVM v,int64_t idx,SQBool *b)
{
	SQObjectPtr &o = stack_get(v, idx);
	if(sq_isbool(o)) {
		*b = _integer(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_getstringandsize(HRABBITVM v,int64_t idx,const SQChar **c,int64_t *size)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_STRING,o);
	*c = _stringval(*o);
	*size = _string(*o)->_len;
	return SQ_OK;
}

SQRESULT sq_getstring(HRABBITVM v,int64_t idx,const SQChar **c)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_STRING,o);
	*c = _stringval(*o);
	return SQ_OK;
}

SQRESULT sq_getthread(HRABBITVM v,int64_t idx,HRABBITVM *thread)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_THREAD,o);
	*thread = _thread(*o);
	return SQ_OK;
}

SQRESULT sq_clone(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &o = stack_get(v,idx);
	v->PushNull();
	if(!v->clone(o, stack_get(v, -1))){
		v->Pop();
		return SQ_ERROR;
	}
	return SQ_OK;
}

int64_t sq_getsize(HRABBITVM v, int64_t idx)
{
	SQObjectPtr &o = stack_get(v, idx);
	SQObjectType type = sq_type(o);
	switch(type) {
	case OT_STRING:	 return _string(o)->_len;
	case OT_TABLE:	  return _table(o)->CountUsed();
	case OT_ARRAY:	  return _array(o)->size();
	case OT_USERDATA:   return _userdata(o)->getsize();
	case OT_INSTANCE:   return _instance(o)->_class->_udsize;
	case OT_CLASS:	  return _class(o)->_udsize;
	default:
		return sq_aux_invalidtype(v, type);
	}
}

SQHash sq_gethash(HRABBITVM v, int64_t idx)
{
	SQObjectPtr &o = stack_get(v, idx);
	return HashObj(o);
}

SQRESULT sq_getuserdata(HRABBITVM v,int64_t idx,SQUserPointer *p,SQUserPointer *typetag)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_USERDATA,o);
	(*p) = _userdataval(*o);
	if(typetag) {
		*typetag = _userdata(*o)->getTypeTag();
	}
	return SQ_OK;
}

SQRESULT sq_settypetag(HRABBITVM v,int64_t idx,SQUserPointer typetag)
{
	SQObjectPtr &o = stack_get(v,idx);
	switch(sq_type(o)) {
		case OT_USERDATA:
			_userdata(o)->setTypeTag(typetag);
			break;
		case OT_CLASS:
			_class(o)->_typetag = typetag;
			break;
		default:
			return sq_throwerror(v,_SC("invalid object type"));
	}
	return SQ_OK;
}

SQRESULT sq_getobjtypetag(const HSQOBJECT *o,SQUserPointer * typetag)
{
  switch(sq_type(*o)) {
	case OT_INSTANCE: *typetag = _instance(*o)->_class->_typetag; break;
	case OT_USERDATA: *typetag = _userdata(*o)->getTypeTag(); break;
	case OT_CLASS:	*typetag = _class(*o)->_typetag; break;
	default: return SQ_ERROR;
  }
  return SQ_OK;
}

SQRESULT sq_gettypetag(HRABBITVM v,int64_t idx,SQUserPointer *typetag)
{
	SQObjectPtr &o = stack_get(v,idx);
	if (SQ_FAILED(sq_getobjtypetag(&o, typetag)))
		return SQ_ERROR;// this is not an error it should be a bool but would break backward compatibility
	return SQ_OK;
}

SQRESULT sq_getuserpointer(HRABBITVM v, int64_t idx, SQUserPointer *p)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_USERPOINTER,o);
	(*p) = _userpointer(*o);
	return SQ_OK;
}

SQRESULT sq_setinstanceup(HRABBITVM v, int64_t idx, SQUserPointer p)
{
	SQObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != OT_INSTANCE) return sq_throwerror(v,_SC("the object is not a class instance"));
	_instance(o)->_userpointer = p;
	return SQ_OK;
}

SQRESULT sq_setclassudsize(HRABBITVM v, int64_t idx, int64_t udsize)
{
	SQObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != OT_CLASS) return sq_throwerror(v,_SC("the object is not a class"));
	if(_class(o)->_locked) return sq_throwerror(v,_SC("the class is locked"));
	_class(o)->_udsize = udsize;
	return SQ_OK;
}


SQRESULT sq_getinstanceup(HRABBITVM v, int64_t idx, SQUserPointer *p,SQUserPointer typetag)
{
	SQObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != OT_INSTANCE) return sq_throwerror(v,_SC("the object is not a class instance"));
	(*p) = _instance(o)->_userpointer;
	if(typetag != 0) {
		SQClass *cl = _instance(o)->_class;
		do{
			if(cl->_typetag == typetag)
				return SQ_OK;
			cl = cl->_base;
		}while(cl != NULL);
		return sq_throwerror(v,_SC("invalid type tag"));
	}
	return SQ_OK;
}

int64_t sq_gettop(HRABBITVM v)
{
	return (v->_top) - v->_stackbase;
}

void sq_settop(HRABBITVM v, int64_t newtop)
{
	int64_t top = sq_gettop(v);
	if(top > newtop)
		sq_pop(v, top - newtop);
	else
		while(top++ < newtop) sq_pushnull(v);
}

void sq_pop(HRABBITVM v, int64_t nelemstopop)
{
	assert(v->_top >= nelemstopop);
	v->Pop(nelemstopop);
}

void sq_poptop(HRABBITVM v)
{
	assert(v->_top >= 1);
	v->Pop();
}


void sq_remove(HRABBITVM v, int64_t idx)
{
	v->remove(idx);
}

int64_t sq_cmp(HRABBITVM v)
{
	int64_t res;
	v->ObjCmp(stack_get(v, -1), stack_get(v, -2),res);
	return res;
}

SQRESULT sq_newslot(HRABBITVM v, int64_t idx, SQBool bstatic)
{
	sq_aux_paramscheck(v, 3);
	SQObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) == OT_TABLE || sq_type(self) == OT_CLASS) {
		SQObjectPtr &key = v->getUp(-2);
		if(sq_type(key) == OT_NULL) return sq_throwerror(v, _SC("null is not a valid key"));
		v->NewSlot(self, key, v->getUp(-1),bstatic?true:false);
		v->Pop(2);
	}
	return SQ_OK;
}

SQRESULT sq_deleteslot(HRABBITVM v,int64_t idx,SQBool pushval)
{
	sq_aux_paramscheck(v, 2);
	SQObjectPtr *self;
	_GETSAFE_OBJ(v, idx, OT_TABLE,self);
	SQObjectPtr &key = v->getUp(-1);
	if(sq_type(key) == OT_NULL) return sq_throwerror(v, _SC("null is not a valid key"));
	SQObjectPtr res;
	if(!v->DeleteSlot(*self, key, res)){
		v->Pop();
		return SQ_ERROR;
	}
	if(pushval) v->getUp(-1) = res;
	else v->Pop();
	return SQ_OK;
}

SQRESULT sq_set(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &self = stack_get(v, idx);
	if(v->set(self, v->getUp(-2), v->getUp(-1),DONT_FALL_BACK)) {
		v->Pop(2);
		return SQ_OK;
	}
	return SQ_ERROR;
}

SQRESULT sq_rawset(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &self = stack_get(v, idx);
	SQObjectPtr &key = v->getUp(-2);
	if(sq_type(key) == OT_NULL) {
		v->Pop(2);
		return sq_throwerror(v, _SC("null key"));
	}
	switch(sq_type(self)) {
	case OT_TABLE:
		_table(self)->NewSlot(key, v->getUp(-1));
		v->Pop(2);
		return SQ_OK;
	break;
	case OT_CLASS:
		_class(self)->NewSlot(_ss(v), key, v->getUp(-1),false);
		v->Pop(2);
		return SQ_OK;
	break;
	case OT_INSTANCE:
		if(_instance(self)->set(key, v->getUp(-1))) {
			v->Pop(2);
			return SQ_OK;
		}
	break;
	case OT_ARRAY:
		if(v->set(self, key, v->getUp(-1),false)) {
			v->Pop(2);
			return SQ_OK;
		}
	break;
	default:
		v->Pop(2);
		return sq_throwerror(v, _SC("rawset works only on array/table/class and instance"));
	}
	v->Raise_IdxError(v->getUp(-2));return SQ_ERROR;
}

SQRESULT sq_newmember(HRABBITVM v,int64_t idx,SQBool bstatic)
{
	SQObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) != OT_CLASS) return sq_throwerror(v, _SC("new member only works with classes"));
	SQObjectPtr &key = v->getUp(-3);
	if(sq_type(key) == OT_NULL) return sq_throwerror(v, _SC("null key"));
	if(!v->NewSlotA(self,key,v->getUp(-2),v->getUp(-1),bstatic?true:false,false)) {
		v->Pop(3);
		return SQ_ERROR;
	}
	v->Pop(3);
	return SQ_OK;
}

SQRESULT sq_rawnewmember(HRABBITVM v,int64_t idx,SQBool bstatic)
{
	SQObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) != OT_CLASS) return sq_throwerror(v, _SC("new member only works with classes"));
	SQObjectPtr &key = v->getUp(-3);
	if(sq_type(key) == OT_NULL) return sq_throwerror(v, _SC("null key"));
	if(!v->NewSlotA(self,key,v->getUp(-2),v->getUp(-1),bstatic?true:false,true)) {
		v->Pop(3);
		return SQ_ERROR;
	}
	v->Pop(3);
	return SQ_OK;
}

SQRESULT sq_setdelegate(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &self = stack_get(v, idx);
	SQObjectPtr &mt = v->getUp(-1);
	SQObjectType type = sq_type(self);
	switch(type) {
	case OT_TABLE:
		if(sq_type(mt) == OT_TABLE) {
			if(!_table(self)->setDelegate(_table(mt))) {
				return sq_throwerror(v, _SC("delagate cycle"));
			}
			v->Pop();
		}
		else if(sq_type(mt)==OT_NULL) {
			_table(self)->setDelegate(NULL); v->Pop(); }
		else return sq_aux_invalidtype(v,type);
		break;
	case OT_USERDATA:
		if(sq_type(mt)==OT_TABLE) {
			_userdata(self)->setDelegate(_table(mt)); v->Pop(); }
		else if(sq_type(mt)==OT_NULL) {
			_userdata(self)->setDelegate(NULL); v->Pop(); }
		else return sq_aux_invalidtype(v, type);
		break;
	default:
			return sq_aux_invalidtype(v, type);
		break;
	}
	return SQ_OK;
}

SQRESULT sq_rawdeleteslot(HRABBITVM v,int64_t idx,SQBool pushval)
{
	sq_aux_paramscheck(v, 2);
	SQObjectPtr *self;
	_GETSAFE_OBJ(v, idx, OT_TABLE,self);
	SQObjectPtr &key = v->getUp(-1);
	SQObjectPtr t;
	if(_table(*self)->get(key,t)) {
		_table(*self)->remove(key);
	}
	if(pushval != 0)
		v->getUp(-1) = t;
	else
		v->Pop();
	return SQ_OK;
}

SQRESULT sq_getdelegate(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(sq_type(self)){
	case OT_TABLE:
	case OT_USERDATA:
		if(!_delegable(self)->_delegate){
			v->PushNull();
			break;
		}
		v->Push(SQObjectPtr(_delegable(self)->_delegate));
		break;
	default: return sq_throwerror(v,_SC("wrong type")); break;
	}
	return SQ_OK;

}

SQRESULT sq_get(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	SQObjectPtr &obj = v->getUp(-1);
	if(v->get(self,obj,obj,false,DONT_FALL_BACK))
		return SQ_OK;
	v->Pop();
	return SQ_ERROR;
}

SQRESULT sq_rawget(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &self=stack_get(v,idx);
	SQObjectPtr &obj = v->getUp(-1);
	switch(sq_type(self)) {
	case OT_TABLE:
		if(_table(self)->get(obj,obj))
			return SQ_OK;
		break;
	case OT_CLASS:
		if(_class(self)->get(obj,obj))
			return SQ_OK;
		break;
	case OT_INSTANCE:
		if(_instance(self)->get(obj,obj))
			return SQ_OK;
		break;
	case OT_ARRAY:{
		if(sq_isnumeric(obj)){
			if(_array(self)->get(tointeger(obj),obj)) {
				return SQ_OK;
			}
		}
		else {
			v->Pop();
			return sq_throwerror(v,_SC("invalid index type for an array"));
		}
				  }
		break;
	default:
		v->Pop();
		return sq_throwerror(v,_SC("rawget works only on array/table/instance and class"));
	}
	v->Pop();
	return sq_throwerror(v,_SC("the index doesn't exist"));
}

SQRESULT sq_getstackobj(HRABBITVM v,int64_t idx,HSQOBJECT *po)
{
	*po=stack_get(v,idx);
	return SQ_OK;
}

const SQChar *sq_getlocal(HRABBITVM v,uint64_t level,uint64_t idx)
{
	uint64_t cstksize=v->_callsstacksize;
	uint64_t lvl=(cstksize-level)-1;
	int64_t stackbase=v->_stackbase;
	if(lvl<cstksize){
		for(uint64_t i=0;i<level;i++){
			SQVM::CallInfo &ci=v->_callsstack[(cstksize-i)-1];
			stackbase-=ci._prevstkbase;
		}
		SQVM::CallInfo &ci=v->_callsstack[lvl];
		if(sq_type(ci._closure)!=OT_CLOSURE)
			return NULL;
		SQClosure *c=_closure(ci._closure);
		SQFunctionProto *func=c->_function;
		if(func->_noutervalues > (int64_t)idx) {
			v->Push(*_outer(c->_outervalues[idx])->_valptr);
			return _stringval(func->_outervalues[idx]._name);
		}
		idx -= func->_noutervalues;
		return func->getLocal(v,stackbase,idx,(int64_t)(ci._ip-func->_instructions)-1);
	}
	return NULL;
}

void sq_pushobject(HRABBITVM v,HSQOBJECT obj)
{
	v->Push(SQObjectPtr(obj));
}

void sq_resetobject(HSQOBJECT *po)
{
	po->_unVal.pUserPointer=NULL;po->_type=OT_NULL;
}

SQRESULT sq_throwerror(HRABBITVM v,const SQChar *err)
{
	v->_lasterror=SQString::create(_ss(v),err);
	return SQ_ERROR;
}

SQRESULT sq_throwobject(HRABBITVM v)
{
	v->_lasterror = v->getUp(-1);
	v->Pop();
	return SQ_ERROR;
}


void sq_reseterror(HRABBITVM v)
{
	v->_lasterror.Null();
}

void sq_getlasterror(HRABBITVM v)
{
	v->Push(v->_lasterror);
}

SQRESULT sq_reservestack(HRABBITVM v,int64_t nsize)
{
	if (((uint64_t)v->_top + nsize) > v->_stack.size()) {
		if(v->_nmetamethodscall) {
			return sq_throwerror(v,_SC("cannot resize stack while in a metamethod"));
		}
		v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
	}
	return SQ_OK;
}

SQRESULT sq_resume(HRABBITVM v,SQBool retval,SQBool raiseerror)
{
	if (sq_type(v->getUp(-1)) == OT_GENERATOR)
	{
		v->PushNull(); //retval
		if (!v->Execute(v->getUp(-2), 0, v->_top, v->getUp(-1), raiseerror, SQVM::ET_RESUME_GENERATOR))
		{v->Raise_Error(v->_lasterror); return SQ_ERROR;}
		if(!retval)
			v->Pop();
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("only generators can be resumed"));
}

SQRESULT sq_call(HRABBITVM v,int64_t params,SQBool retval,SQBool raiseerror)
{
	SQObjectPtr res;
	if(v->Call(v->getUp(-(params+1)),params,v->_top-params,res,raiseerror?true:false)){

		if(!v->_suspended) {
			v->Pop(params);//pop args
		}
		if(retval){
			v->Push(res); return SQ_OK;
		}
		return SQ_OK;
	}
	else {
		v->Pop(params);
		return SQ_ERROR;
	}
	if(!v->_suspended)
		v->Pop(params);
	return sq_throwerror(v,_SC("call failed"));
}

SQRESULT sq_tailcall(HRABBITVM v, int64_t nparams)
{
	SQObjectPtr &res = v->getUp(-(nparams + 1));
	if (sq_type(res) != OT_CLOSURE) {
		return sq_throwerror(v, _SC("only closure can be tail called"));
	}
	SQClosure *clo = _closure(res);
	if (clo->_function->_bgenerator)
	{
		return sq_throwerror(v, _SC("generators cannot be tail called"));
	}
	
	int64_t stackbase = (v->_top - nparams) - v->_stackbase;
	if (!v->TailCall(clo, stackbase, nparams)) {
		return SQ_ERROR;
	}
	return SQ_TAILCALL_FLAG;
}

SQRESULT sq_suspendvm(HRABBITVM v)
{
	return v->Suspend();
}

SQRESULT sq_wakeupvm(HRABBITVM v,SQBool wakeupret,SQBool retval,SQBool raiseerror,SQBool throwerror)
{
	SQObjectPtr ret;
	if(!v->_suspended)
		return sq_throwerror(v,_SC("cannot resume a vm that is not running any code"));
	int64_t target = v->_suspended_target;
	if(wakeupret) {
		if(target != -1) {
			v->getAt(v->_stackbase+v->_suspended_target)=v->getUp(-1); //retval
		}
		v->Pop();
	} else if(target != -1) { v->getAt(v->_stackbase+v->_suspended_target).Null(); }
	SQObjectPtr dummy;
	if(!v->Execute(dummy,-1,-1,ret,raiseerror,throwerror?SQVM::ET_RESUME_THROW_VM : SQVM::ET_RESUME_VM)) {
		return SQ_ERROR;
	}
	if(retval) {
		v->Push(ret);
	}
	return SQ_OK;
}

void sq_setreleasehook(HRABBITVM v,int64_t idx,SQRELEASEHOOK hook)
{
	SQObjectPtr &ud=stack_get(v,idx);
	switch(sq_type(ud) ) {
		case OT_USERDATA:
			_userdata(ud)->setHook(hook);
			break;
		case OT_INSTANCE:
			_instance(ud)->_hook = hook;
			break;
		case OT_CLASS:
			_class(ud)->_hook = hook;
			break;
		default:
			return;
	}
}

SQRELEASEHOOK sq_getreleasehook(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &ud=stack_get(v,idx);
	switch(sq_type(ud) ) {
		case OT_USERDATA:
			return _userdata(ud)->getHook();
			break;
		case OT_INSTANCE:
			return _instance(ud)->_hook;
			break;
		case OT_CLASS:
			return _class(ud)->_hook;
			break;
		default:
			return NULL;
	}
}

void sq_setcompilererrorhandler(HRABBITVM v,SQCOMPILERERROR f)
{
	_ss(v)->_compilererrorhandler = f;
}

SQRESULT sq_writeclosure(HRABBITVM v,SQWRITEFUNC w,SQUserPointer up)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, -1, OT_CLOSURE,o);
	unsigned short tag = SQ_BYTECODE_STREAM_TAG;
	if(_closure(*o)->_function->_noutervalues)
		return sq_throwerror(v,_SC("a closure with free variables bound cannot be serialized"));
	if(w(up,&tag,2) != 2)
		return sq_throwerror(v,_SC("io error"));
	if(!_closure(*o)->Save(v,up,w))
		return SQ_ERROR;
	return SQ_OK;
}

SQRESULT sq_readclosure(HRABBITVM v,SQREADFUNC r,SQUserPointer up)
{
	SQObjectPtr closure;

	unsigned short tag;
	if(r(up,&tag,2) != 2)
		return sq_throwerror(v,_SC("io error"));
	if(tag != SQ_BYTECODE_STREAM_TAG)
		return sq_throwerror(v,_SC("invalid stream"));
	if(!SQClosure::Load(v,up,r,closure))
		return SQ_ERROR;
	v->Push(closure);
	return SQ_OK;
}

SQChar *sq_getscratchpad(HRABBITVM v,int64_t minsize)
{
	return _ss(v)->getScratchPad(minsize);
}

SQRESULT sq_resurrectunreachable(HRABBITVM v)
{
	return sq_throwerror(v,_SC("sq_resurrectunreachable requires a garbage collector build"));
}

// TODO: remove this...
int64_t sq_collectgarbage(HRABBITVM v)
{
	return -1;
}

SQRESULT sq_getcallee(HRABBITVM v)
{
	if(v->_callsstacksize > 1)
	{
		v->Push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("no closure in the calls stack"));
}

const SQChar *sq_getfreevariable(HRABBITVM v,int64_t idx,uint64_t nval)
{
	SQObjectPtr &self=stack_get(v,idx);
	const SQChar *name = NULL;
	switch(sq_type(self))
	{
	case OT_CLOSURE:{
		SQClosure *clo = _closure(self);
		SQFunctionProto *fp = clo->_function;
		if(((uint64_t)fp->_noutervalues) > nval) {
			v->Push(*(_outer(clo->_outervalues[nval])->_valptr));
			SQOuterVar &ov = fp->_outervalues[nval];
			name = _stringval(ov._name);
		}
					}
		break;
	case OT_NATIVECLOSURE:{
		SQNativeClosure *clo = _nativeclosure(self);
		if(clo->_noutervalues > nval) {
			v->Push(clo->_outervalues[nval]);
			name = _SC("@NATIVE");
		}
						  }
		break;
	default: break; //shutup compiler
	}
	return name;
}

SQRESULT sq_setfreevariable(HRABBITVM v,int64_t idx,uint64_t nval)
{
	SQObjectPtr &self=stack_get(v,idx);
	switch(sq_type(self))
	{
	case OT_CLOSURE:{
		SQFunctionProto *fp = _closure(self)->_function;
		if(((uint64_t)fp->_noutervalues) > nval){
			*(_outer(_closure(self)->_outervalues[nval])->_valptr) = stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
					}
		break;
	case OT_NATIVECLOSURE:
		if(_nativeclosure(self)->_noutervalues > nval){
			_nativeclosure(self)->_outervalues[nval] = stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
		break;
	default:
		return sq_aux_invalidtype(v, sq_type(self));
	}
	v->Pop();
	return SQ_OK;
}

SQRESULT sq_setattributes(HRABBITVM v,int64_t idx)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS,o);
	SQObjectPtr &key = stack_get(v,-2);
	SQObjectPtr &val = stack_get(v,-1);
	SQObjectPtr attrs;
	if(sq_type(key) == OT_NULL) {
		attrs = _class(*o)->_attributes;
		_class(*o)->_attributes = val;
		v->Pop(2);
		v->Push(attrs);
		return SQ_OK;
	}else if(_class(*o)->getAttributes(key,attrs)) {
		_class(*o)->setAttributes(key,val);
		v->Pop(2);
		v->Push(attrs);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("wrong index"));
}

SQRESULT sq_getattributes(HRABBITVM v,int64_t idx)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS,o);
	SQObjectPtr &key = stack_get(v,-1);
	SQObjectPtr attrs;
	if(sq_type(key) == OT_NULL) {
		attrs = _class(*o)->_attributes;
		v->Pop();
		v->Push(attrs);
		return SQ_OK;
	}
	else if(_class(*o)->getAttributes(key,attrs)) {
		v->Pop();
		v->Push(attrs);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("wrong index"));
}

SQRESULT sq_getmemberhandle(HRABBITVM v,int64_t idx,HSQMEMBERHANDLE *handle)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS,o);
	SQObjectPtr &key = stack_get(v,-1);
	SQTable *m = _class(*o)->_members;
	SQObjectPtr val;
	if(m->get(key,val)) {
		handle->_static = _isfield(val) ? SQFalse : SQTrue;
		handle->_index = _member_idx(val);
		v->Pop();
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("wrong index"));
}

SQRESULT _getmemberbyhandle(HRABBITVM v,SQObjectPtr &self,const HSQMEMBERHANDLE *handle,SQObjectPtr *&val)
{
	switch(sq_type(self)) {
		case OT_INSTANCE: {
				SQInstance *i = _instance(self);
				if(handle->_static) {
					SQClass *c = i->_class;
					val = &c->_methods[handle->_index].val;
				}
				else {
					val = &i->_values[handle->_index];

				}
			}
			break;
		case OT_CLASS: {
				SQClass *c = _class(self);
				if(handle->_static) {
					val = &c->_methods[handle->_index].val;
				}
				else {
					val = &c->_defaultvalues[handle->_index].val;
				}
			}
			break;
		default:
			return sq_throwerror(v,_SC("wrong type(expected class or instance)"));
	}
	return SQ_OK;
}

SQRESULT sq_getbyhandle(HRABBITVM v,int64_t idx,const HSQMEMBERHANDLE *handle)
{
	SQObjectPtr &self = stack_get(v,idx);
	SQObjectPtr *val = NULL;
	if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
		return SQ_ERROR;
	}
	v->Push(_realval(*val));
	return SQ_OK;
}

SQRESULT sq_setbyhandle(HRABBITVM v,int64_t idx,const HSQMEMBERHANDLE *handle)
{
	SQObjectPtr &self = stack_get(v,idx);
	SQObjectPtr &newval = stack_get(v,-1);
	SQObjectPtr *val = NULL;
	if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
		return SQ_ERROR;
	}
	*val = newval;
	v->Pop();
	return SQ_OK;
}

SQRESULT sq_getbase(HRABBITVM v,int64_t idx)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS,o);
	if(_class(*o)->_base)
		v->Push(SQObjectPtr(_class(*o)->_base));
	else
		v->PushNull();
	return SQ_OK;
}

SQRESULT sq_getclass(HRABBITVM v,int64_t idx)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_INSTANCE,o);
	v->Push(SQObjectPtr(_instance(*o)->_class));
	return SQ_OK;
}

SQRESULT sq_createinstance(HRABBITVM v,int64_t idx)
{
	SQObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, OT_CLASS,o);
	v->Push(_class(*o)->createInstance());
	return SQ_OK;
}

void sq_weakref(HRABBITVM v,int64_t idx)
{
	SQObject &o=stack_get(v,idx);
	if(ISREFCOUNTED(sq_type(o))) {
		v->Push(_refcounted(o)->getWeakRef(sq_type(o)));
		return;
	}
	v->Push(o);
}

SQRESULT sq_getweakrefval(HRABBITVM v,int64_t idx)
{
	SQObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != OT_WEAKREF) {
		return sq_throwerror(v,_SC("the object must be a weakref"));
	}
	v->Push(_weakref(o)->_obj);
	return SQ_OK;
}

SQRESULT sq_getdefaultdelegate(HRABBITVM v,SQObjectType t)
{
	SQSharedState *ss = _ss(v);
	switch(t) {
	case OT_TABLE: v->Push(ss->_table_default_delegate); break;
	case OT_ARRAY: v->Push(ss->_array_default_delegate); break;
	case OT_STRING: v->Push(ss->_string_default_delegate); break;
	case OT_INTEGER: case OT_FLOAT: v->Push(ss->_number_default_delegate); break;
	case OT_GENERATOR: v->Push(ss->_generator_default_delegate); break;
	case OT_CLOSURE: case OT_NATIVECLOSURE: v->Push(ss->_closure_default_delegate); break;
	case OT_THREAD: v->Push(ss->_thread_default_delegate); break;
	case OT_CLASS: v->Push(ss->_class_default_delegate); break;
	case OT_INSTANCE: v->Push(ss->_instance_default_delegate); break;
	case OT_WEAKREF: v->Push(ss->_weakref_default_delegate); break;
	default: return sq_throwerror(v,_SC("the type doesn't have a default delegate"));
	}
	return SQ_OK;
}

SQRESULT sq_next(HRABBITVM v,int64_t idx)
{
	SQObjectPtr o=stack_get(v,idx),&refpos = stack_get(v,-1),realkey,val;
	if(sq_type(o) == OT_GENERATOR) {
		return sq_throwerror(v,_SC("cannot iterate a generator"));
	}
	int faketojump;
	if(!v->FOREACH_OP(o,realkey,val,refpos,0,666,faketojump))
		return SQ_ERROR;
	if(faketojump != 666) {
		v->Push(realkey);
		v->Push(val);
		return SQ_OK;
	}
	return SQ_ERROR;
}

struct BufState{
	const SQChar *buf;
	int64_t ptr;
	int64_t size;
};

int64_t buf_lexfeed(SQUserPointer file)
{
	BufState *buf=(BufState*)file;
	if(buf->size<(buf->ptr+1))
		return 0;
	return buf->buf[buf->ptr++];
}

SQRESULT sq_compilebuffer(HRABBITVM v,const SQChar *s,int64_t size,const SQChar *sourcename,SQBool raiseerror) {
	BufState buf;
	buf.buf = s;
	buf.size = size;
	buf.ptr = 0;
	return sq_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void sq_move(HRABBITVM dest,HRABBITVM src,int64_t idx)
{
	dest->Push(stack_get(src,idx));
}

void sq_setprintfunc(HRABBITVM v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc)
{
	_ss(v)->_printfunc = printfunc;
	_ss(v)->_errorfunc = errfunc;
}

SQPRINTFUNCTION sq_getprintfunc(HRABBITVM v)
{
	return _ss(v)->_printfunc;
}

SQPRINTFUNCTION sq_geterrorfunc(HRABBITVM v)
{
	return _ss(v)->_errorfunc;
}

void *sq_malloc(uint64_t size)
{
	return SQ_MALLOC(size);
}

void *sq_realloc(void* p,uint64_t oldsize,uint64_t newsize)
{
	return SQ_REALLOC(p,oldsize,newsize);
}

void sq_free(void *p,uint64_t size)
{
	SQ_FREE(p,size);
}
