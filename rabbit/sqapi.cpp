/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/sqpcheader.hpp>
#include <rabbit/VirtualMachine.hpp>


#include <rabbit/Array.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/UserData.hpp>
#include <rabbit/sqcompiler.hpp>
#include <rabbit/sqfuncstate.hpp>
#include <rabbit/Instance.hpp>

#include <rabbit/MemberHandle.hpp>

static bool sq_aux_gettypedarg(rabbit::VirtualMachine* v,int64_t idx,rabbit::ObjectType type,rabbit::ObjectPtr **o)
{
	*o = &stack_get(v,idx);
	if(sq_type(**o) != type){
		rabbit::ObjectPtr oval = v->printObjVal(**o);
		v->raise_error(_SC("wrong argument type, expected '%s' got '%.50s'"),IdType2Name(type),_stringval(oval));
		return false;
	}
	return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { if(!sq_aux_gettypedarg(v,idx,type,&o)) return SQ_ERROR; }

#define sq_aux_paramscheck(v,count) \
{ \
	if(sq_gettop(v) < count){ v->raise_error(_SC("not enough params in the stack")); return SQ_ERROR; }\
}


int64_t sq_aux_invalidtype(rabbit::VirtualMachine* v,rabbit::ObjectType type)
{
	uint64_t buf_size = 100 *sizeof(rabbit::Char);
	scsprintf(_get_shared_state(v)->getScratchPad(buf_size), buf_size, _SC("unexpected type %s"), IdType2Name(type));
	return sq_throwerror(v, _get_shared_state(v)->getScratchPad(-1));
}

rabbit::VirtualMachine* sq_open(int64_t initialstacksize)
{
	rabbit::SharedState *ss;
	sq_new(ss, rabbit::SharedState);
	ss->init();
	
	char* allocatedData = (char*)SQ_MALLOC(sizeof(rabbit::VirtualMachine));
	rabbit::VirtualMachine *v = new (allocatedData) rabbit::VirtualMachine(ss);
	ss->_root_vm = v;
	
	if(v->init(NULL, initialstacksize)) {
		return v;
	} else {
		v->~VirtualMachine();
		SQ_FREE(allocatedData,sizeof(rabbit::VirtualMachine));
		return NULL;
	}
	return v;
}

rabbit::VirtualMachine* sq_newthread(rabbit::VirtualMachine* friendvm, int64_t initialstacksize)
{
	rabbit::SharedState *ss;
	ss=_get_shared_state(friendvm);
	
	char* allocatedData = (char*)SQ_MALLOC(sizeof(rabbit::VirtualMachine));
	rabbit::VirtualMachine *v = new (allocatedData) rabbit::VirtualMachine(ss);
	ss->_root_vm = v;
	
	if(v->init(friendvm, initialstacksize)) {
		friendvm->push(v);
		return v;
	} else {
		v->~VirtualMachine();
		SQ_FREE(allocatedData,sizeof(rabbit::VirtualMachine));
		return NULL;
	}
}

int64_t sq_getvmstate(rabbit::VirtualMachine* v)
{
	if(v->_suspended)
		return SQ_VMSTATE_SUSPENDED;
	else {
		if(v->_callsstacksize != 0) return SQ_VMSTATE_RUNNING;
		else return SQ_VMSTATE_IDLE;
	}
}

void sq_seterrorhandler(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v, -1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
		v->_errorhandler = o;
		v->pop();
	}
}

void sq_setnativedebughook(rabbit::VirtualMachine* v,SQDEBUGHOOK hook)
{
	v->_debughook_native = hook;
	v->_debughook_closure.Null();
	v->_debughook = hook?true:false;
}

void sq_setdebughook(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v,-1);
	if(sq_isclosure(o) || sq_isnativeclosure(o) || sq_isnull(o)) {
		v->_debughook_closure = o;
		v->_debughook_native = NULL;
		v->_debughook = !sq_isnull(o);
		v->pop();
	}
}

void sq_close(rabbit::VirtualMachine* v)
{
	rabbit::SharedState *ss = _get_shared_state(v);
	_thread(ss->_root_vm)->finalize();
	sq_delete(ss, rabbit::SharedState);
}

int64_t sq_getversion()
{
	return RABBIT_VERSION_NUMBER;
}

rabbit::Result sq_compile(rabbit::VirtualMachine* v,SQLEXREADFUNC read,rabbit::UserPointer p,const rabbit::Char *sourcename,rabbit::Bool raiseerror)
{
	rabbit::ObjectPtr o;
#ifndef NO_COMPILER
	if(compile(v, read, p, sourcename, o, raiseerror?true:false, _get_shared_state(v)->_debuginfo)) {
		v->push(SQClosure::create(_get_shared_state(v), _funcproto(o), _table(v->_roottable)->getWeakRef(rabbit::OT_TABLE)));
		return SQ_OK;
	}
	return SQ_ERROR;
#else
	return sq_throwerror(v,_SC("this is a no compiler build"));
#endif
}

void sq_enabledebuginfo(rabbit::VirtualMachine* v, rabbit::Bool enable)
{
	_get_shared_state(v)->_debuginfo = enable?true:false;
}

void sq_notifyallexceptions(rabbit::VirtualMachine* v, rabbit::Bool enable)
{
	_get_shared_state(v)->_notifyallexceptions = enable?true:false;
}

void sq_addref(rabbit::VirtualMachine* v,rabbit::Object *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return;
	__addRef(po->_type,po->_unVal);
}

uint64_t sq_getrefcount(rabbit::VirtualMachine* v,rabbit::Object *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return 0;
   return po->_unVal.pRefCounted->refCountget();
}

rabbit::Bool sq_release(rabbit::VirtualMachine* v,rabbit::Object *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return SQTrue;
	bool ret = (po->_unVal.pRefCounted->refCountget() <= 1) ? SQTrue : SQFalse;
	__release(po->_type,po->_unVal);
	return ret; //the ret val doesn't work(and cannot be fixed)
}

uint64_t sq_getvmrefcount(rabbit::VirtualMachine* SQ_UNUSED_ARG(v), const rabbit::Object *po)
{
	if (!ISREFCOUNTED(sq_type(*po))) return 0;
	return po->_unVal.pRefCounted->refCountget();
}

const rabbit::Char *sq_objtostring(const rabbit::Object *o)
{
	if(sq_type(*o) == rabbit::OT_STRING) {
		return _stringval(*o);
	}
	return NULL;
}

int64_t sq_objtointeger(const rabbit::Object *o)
{
	if(sq_isnumeric(*o)) {
		return tointeger(*o);
	}
	return 0;
}

float_t sq_objtofloat(const rabbit::Object *o)
{
	if(sq_isnumeric(*o)) {
		return tofloat(*o);
	}
	return 0;
}

rabbit::Bool sq_objtobool(const rabbit::Object *o)
{
	if(sq_isbool(*o)) {
		return _integer(*o);
	}
	return SQFalse;
}

rabbit::UserPointer sq_objtouserpointer(const rabbit::Object *o)
{
	if(sq_isuserpointer(*o)) {
		return _userpointer(*o);
	}
	return 0;
}

void sq_pushnull(rabbit::VirtualMachine* v)
{
	v->pushNull();
}

void sq_pushstring(rabbit::VirtualMachine* v,const rabbit::Char *s,int64_t len)
{
	if(s)
		v->push(rabbit::ObjectPtr(rabbit::String::create(_get_shared_state(v), s, len)));
	else v->pushNull();
}

void sq_pushinteger(rabbit::VirtualMachine* v,int64_t n)
{
	v->push(n);
}

void sq_pushbool(rabbit::VirtualMachine* v,rabbit::Bool b)
{
	v->push(b?true:false);
}

void sq_pushfloat(rabbit::VirtualMachine* v,float_t n)
{
	v->push(n);
}

void sq_pushuserpointer(rabbit::VirtualMachine* v,rabbit::UserPointer p)
{
	v->push(p);
}

void sq_pushthread(rabbit::VirtualMachine* v, rabbit::VirtualMachine* thread)
{
	v->push(thread);
}

rabbit::UserPointer sq_newuserdata(rabbit::VirtualMachine* v,uint64_t size)
{
	rabbit::UserData *ud = rabbit::UserData::create(_get_shared_state(v), size + SQ_ALIGNMENT);
	v->push(ud);
	return (rabbit::UserPointer)sq_aligning(ud + 1);
}

void sq_newtable(rabbit::VirtualMachine* v)
{
	v->push(rabbit::Table::create(_get_shared_state(v), 0));
}

void sq_newtableex(rabbit::VirtualMachine* v,int64_t initialcapacity)
{
	v->push(rabbit::Table::create(_get_shared_state(v), initialcapacity));
}

void sq_newarray(rabbit::VirtualMachine* v,int64_t size)
{
	v->push(rabbit::Array::create(_get_shared_state(v), size));
}

rabbit::Result sq_newclass(rabbit::VirtualMachine* v,rabbit::Bool hasbase)
{
	rabbit::Class *baseclass = NULL;
	if(hasbase) {
		rabbit::ObjectPtr &base = stack_get(v,-1);
		if(sq_type(base) != rabbit::OT_CLASS)
			return sq_throwerror(v,_SC("invalid base type"));
		baseclass = _class(base);
	}
	rabbit::Class *newclass = rabbit::Class::create(_get_shared_state(v), baseclass);
	if(baseclass) v->pop();
	v->push(newclass);
	return SQ_OK;
}

rabbit::Bool sq_instanceof(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &inst = stack_get(v,-1);
	rabbit::ObjectPtr &cl = stack_get(v,-2);
	if(    sq_type(inst) != rabbit::OT_INSTANCE
	    || sq_type(cl) != rabbit::OT_CLASS)
		return sq_throwerror(v,_SC("invalid param type"));
	return _instance(inst)->instanceOf(_class(cl))?SQTrue:SQFalse;
}

rabbit::Result sq_arrayappend(rabbit::VirtualMachine* v,int64_t idx)
{
	sq_aux_paramscheck(v,2);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	_array(*arr)->append(v->getUp(-1));
	v->pop();
	return SQ_OK;
}

rabbit::Result sq_arraypop(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	if(_array(*arr)->size() > 0) {
		if(pushval != 0){
			v->push(_array(*arr)->top());
		}
		_array(*arr)->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("empty array"));
}

rabbit::Result sq_arrayresize(rabbit::VirtualMachine* v,int64_t idx,int64_t newsize)
{
	sq_aux_paramscheck(v,1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	if(newsize >= 0) {
		_array(*arr)->resize(newsize);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("negative size"));
}


rabbit::Result sq_arrayreverse(rabbit::VirtualMachine* v,int64_t idx)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *o;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,o);
	rabbit::Array *arr = _array(*o);
	if(arr->size() > 0) {
		rabbit::ObjectPtr t;
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

rabbit::Result sq_arrayremove(rabbit::VirtualMachine* v,int64_t idx,int64_t itemidx)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	return _array(*arr)->remove(itemidx) ? SQ_OK : sq_throwerror(v,_SC("index out of range"));
}

rabbit::Result sq_arrayinsert(rabbit::VirtualMachine* v,int64_t idx,int64_t destpos)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	rabbit::Result ret = _array(*arr)->insert(destpos, v->getUp(-1)) ? SQ_OK : sq_throwerror(v,_SC("index out of range"));
	v->pop();
	return ret;
}

void sq_newclosure(rabbit::VirtualMachine* v,SQFUNCTION func,uint64_t nfreevars)
{
	SQNativeClosure *nc = SQNativeClosure::create(_get_shared_state(v), func,nfreevars);
	nc->_nparamscheck = 0;
	for(uint64_t i = 0; i < nfreevars; i++) {
		nc->_outervalues[i] = v->top();
		v->pop();
	}
	v->push(rabbit::ObjectPtr(nc));
}

rabbit::Result sq_getclosureinfo(rabbit::VirtualMachine* v,int64_t idx,uint64_t *nparams,uint64_t *nfreevars)
{
	rabbit::Object o = stack_get(v, idx);
	if(sq_type(o) == rabbit::OT_CLOSURE) {
		SQClosure *c = _closure(o);
		SQFunctionProto *proto = c->_function;
		*nparams = (uint64_t)proto->_nparameters;
		*nfreevars = (uint64_t)proto->_noutervalues;
		return SQ_OK;
	}
	else if(sq_type(o) == rabbit::OT_NATIVECLOSURE)
	{
		SQNativeClosure *c = _nativeclosure(o);
		*nparams = (uint64_t)c->_nparamscheck;
		*nfreevars = c->_noutervalues;
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("the object is not a closure"));
}

rabbit::Result sq_setnativeclosurename(rabbit::VirtualMachine* v,int64_t idx,const rabbit::Char *name)
{
	rabbit::Object o = stack_get(v, idx);
	if(sq_isnativeclosure(o)) {
		SQNativeClosure *nc = _nativeclosure(o);
		nc->_name = rabbit::String::create(_get_shared_state(v),name);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("the object is not a nativeclosure"));
}

rabbit::Result sq_setparamscheck(rabbit::VirtualMachine* v,int64_t nparamscheck,const rabbit::Char *typemask)
{
	rabbit::Object o = stack_get(v, -1);
	if(!sq_isnativeclosure(o))
		return sq_throwerror(v, _SC("native closure expected"));
	SQNativeClosure *nc = _nativeclosure(o);
	nc->_nparamscheck = nparamscheck;
	if(typemask) {
		etk::Vector<int64_t> res;
		if(!compileTypemask(res, typemask))
			return sq_throwerror(v, _SC("invalid typemask"));
		nc->_typecheck = res;
	}
	else {
		nc->_typecheck.resize(0);
	}
	if(nparamscheck == SQ_MATCHTYPEMASKSTRING) {
		nc->_nparamscheck = nc->_typecheck.size();
	}
	return SQ_OK;
}

rabbit::Result sq_bindenv(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(!sq_isnativeclosure(o) &&
		!sq_isclosure(o))
		return sq_throwerror(v,_SC("the target is not a closure"));
	rabbit::ObjectPtr &env = stack_get(v,-1);
	if(!sq_istable(env) &&
		!sq_isarray(env) &&
		!sq_isclass(env) &&
		!sq_isinstance(env))
		return sq_throwerror(v,_SC("invalid environment"));
	rabbit::WeakRef *w = _refcounted(env)->getWeakRef(sq_type(env));
	rabbit::ObjectPtr ret;
	if(sq_isclosure(o)) {
		SQClosure *c = _closure(o)->clone();
		__Objrelease(c->_env);
		c->_env = w;
		__ObjaddRef(c->_env);
		if(_closure(o)->_base) {
			c->_base = _closure(o)->_base;
			__ObjaddRef(c->_base);
		}
		ret = c;
	}
	else { //then must be a native closure
		SQNativeClosure *c = _nativeclosure(o)->clone();
		__Objrelease(c->_env);
		c->_env = w;
		__ObjaddRef(c->_env);
		ret = c;
	}
	v->pop();
	v->push(ret);
	return SQ_OK;
}

rabbit::Result sq_getclosurename(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(!sq_isnativeclosure(o) &&
		!sq_isclosure(o))
		return sq_throwerror(v,_SC("the target is not a closure"));
	if(sq_isnativeclosure(o))
	{
		v->push(_nativeclosure(o)->_name);
	}
	else { //closure
		v->push(_closure(o)->_function->_name);
	}
	return SQ_OK;
}

rabbit::Result sq_setclosureroot(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &c = stack_get(v,idx);
	rabbit::Object o = stack_get(v, -1);
	if(!sq_isclosure(c)) return sq_throwerror(v, _SC("closure expected"));
	if(sq_istable(o)) {
		_closure(c)->setRoot(_table(o)->getWeakRef(rabbit::OT_TABLE));
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("invalid type"));
}

rabbit::Result sq_getclosureroot(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &c = stack_get(v,idx);
	if(!sq_isclosure(c)) return sq_throwerror(v, _SC("closure expected"));
	v->push(_closure(c)->_root->_obj);
	return SQ_OK;
}

rabbit::Result sq_clear(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::Object &o=stack_get(v,idx);
	switch(sq_type(o)) {
		case rabbit::OT_TABLE: _table(o)->clear();  break;
		case rabbit::OT_ARRAY: _array(o)->resize(0); break;
		default:
			return sq_throwerror(v, _SC("clear only works on table and array"));
		break;

	}
	return SQ_OK;
}

void sq_pushroottable(rabbit::VirtualMachine* v)
{
	v->push(v->_roottable);
}

void sq_pushregistrytable(rabbit::VirtualMachine* v)
{
	v->push(_get_shared_state(v)->_registry);
}

void sq_pushconsttable(rabbit::VirtualMachine* v)
{
	v->push(_get_shared_state(v)->_consts);
}

rabbit::Result sq_setroottable(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v, -1);
	if(sq_istable(o) || sq_isnull(o)) {
		v->_roottable = o;
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("invalid type"));
}

rabbit::Result sq_setconsttable(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v, -1);
	if(sq_istable(o)) {
		_get_shared_state(v)->_consts = o;
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, _SC("invalid type, expected table"));
}

void sq_setforeignptr(rabbit::VirtualMachine* v,rabbit::UserPointer p)
{
	v->_foreignptr = p;
}

rabbit::UserPointer sq_getforeignptr(rabbit::VirtualMachine* v)
{
	return v->_foreignptr;
}

void sq_setsharedforeignptr(rabbit::VirtualMachine* v,rabbit::UserPointer p)
{
	_get_shared_state(v)->_foreignptr = p;
}

rabbit::UserPointer sq_getsharedforeignptr(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_foreignptr;
}

void sq_setvmreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook)
{
	v->_releasehook = hook;
}

SQRELEASEHOOK sq_getvmreleasehook(rabbit::VirtualMachine* v)
{
	return v->_releasehook;
}

void sq_setsharedreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook)
{
	_get_shared_state(v)->_releasehook = hook;
}

SQRELEASEHOOK sq_getsharedreleasehook(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_releasehook;
}

void sq_push(rabbit::VirtualMachine* v,int64_t idx)
{
	v->push(stack_get(v, idx));
}

rabbit::ObjectType sq_gettype(rabbit::VirtualMachine* v,int64_t idx)
{
	return sq_type(stack_get(v, idx));
}

rabbit::Result sq_typeof(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	rabbit::ObjectPtr res;
	if(!v->typeOf(o,res)) {
		return SQ_ERROR;
	}
	v->push(res);
	return SQ_OK;
}

rabbit::Result sq_tostring(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	rabbit::ObjectPtr res;
	if(!v->toString(o,res)) {
		return SQ_ERROR;
	}
	v->push(res);
	return SQ_OK;
}

void sq_tobool(rabbit::VirtualMachine* v, int64_t idx, rabbit::Bool *b)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	*b = rabbit::VirtualMachine::IsFalse(o)?SQFalse:SQTrue;
}

rabbit::Result sq_getinteger(rabbit::VirtualMachine* v,int64_t idx,int64_t *i)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	if(sq_isnumeric(o)) {
		*i = tointeger(o);
		return SQ_OK;
	}
	if(sq_isbool(o)) {
		*i = rabbit::VirtualMachine::IsFalse(o)?SQFalse:SQTrue;
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result sq_getfloat(rabbit::VirtualMachine* v,int64_t idx,float_t *f)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	if(sq_isnumeric(o)) {
		*f = tofloat(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result sq_getbool(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool *b)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	if(sq_isbool(o)) {
		*b = _integer(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result sq_getstringandsize(rabbit::VirtualMachine* v,int64_t idx,const rabbit::Char **c,int64_t *size)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_STRING,o);
	*c = _stringval(*o);
	*size = _string(*o)->_len;
	return SQ_OK;
}

rabbit::Result sq_getstring(rabbit::VirtualMachine* v,int64_t idx,const rabbit::Char **c)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_STRING,o);
	*c = _stringval(*o);
	return SQ_OK;
}

rabbit::Result sq_getthread(rabbit::VirtualMachine* v,int64_t idx,rabbit::VirtualMachine* *thread)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_THREAD,o);
	*thread = _thread(*o);
	return SQ_OK;
}

rabbit::Result sq_clone(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	v->pushNull();
	if(!v->clone(o, stack_get(v, -1))){
		v->pop();
		return SQ_ERROR;
	}
	return SQ_OK;
}

int64_t sq_getsize(rabbit::VirtualMachine* v, int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	rabbit::ObjectType type = sq_type(o);
	switch(type) {
	case rabbit::OT_STRING:	 return _string(o)->_len;
	case rabbit::OT_TABLE:	  return _table(o)->countUsed();
	case rabbit::OT_ARRAY:	  return _array(o)->size();
	case rabbit::OT_USERDATA:   return _userdata(o)->getsize();
	case rabbit::OT_INSTANCE:   return _instance(o)->_class->_udsize;
	case rabbit::OT_CLASS:	  return _class(o)->_udsize;
	default:
		return sq_aux_invalidtype(v, type);
	}
}

rabbit::Hash sq_gethash(rabbit::VirtualMachine* v, int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	return HashObj(o);
}

rabbit::Result sq_getuserdata(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *p,rabbit::UserPointer *typetag)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_USERDATA,o);
	(*p) = _userdataval(*o);
	if(typetag) {
		*typetag = _userdata(*o)->getTypeTag();
	}
	return SQ_OK;
}

rabbit::Result sq_settypetag(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer typetag)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	switch(sq_type(o)) {
		case rabbit::OT_USERDATA:
			_userdata(o)->setTypeTag(typetag);
			break;
		case rabbit::OT_CLASS:
			_class(o)->_typetag = typetag;
			break;
		default:
			return sq_throwerror(v,_SC("invalid object type"));
	}
	return SQ_OK;
}

rabbit::Result sq_getobjtypetag(const rabbit::Object *o,rabbit::UserPointer * typetag)
{
  switch(sq_type(*o)) {
	case rabbit::OT_INSTANCE: *typetag = _instance(*o)->_class->_typetag; break;
	case rabbit::OT_USERDATA: *typetag = _userdata(*o)->getTypeTag(); break;
	case rabbit::OT_CLASS:	*typetag = _class(*o)->_typetag; break;
	default: return SQ_ERROR;
  }
  return SQ_OK;
}

rabbit::Result sq_gettypetag(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *typetag)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if (SQ_FAILED(sq_getobjtypetag(&o, typetag)))
		return SQ_ERROR;// this is not an error it should be a bool but would break backward compatibility
	return SQ_OK;
}

rabbit::Result sq_getuserpointer(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer *p)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_USERPOINTER,o);
	(*p) = _userpointer(*o);
	return SQ_OK;
}

rabbit::Result sq_setinstanceup(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer p)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_INSTANCE) return sq_throwerror(v,_SC("the object is not a class instance"));
	_instance(o)->_userpointer = p;
	return SQ_OK;
}

rabbit::Result sq_setclassudsize(rabbit::VirtualMachine* v, int64_t idx, int64_t udsize)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_CLASS) return sq_throwerror(v,_SC("the object is not a class"));
	if(_class(o)->_locked) return sq_throwerror(v,_SC("the class is locked"));
	_class(o)->_udsize = udsize;
	return SQ_OK;
}


rabbit::Result sq_getinstanceup(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer *p,rabbit::UserPointer typetag)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_INSTANCE) return sq_throwerror(v,_SC("the object is not a class instance"));
	(*p) = _instance(o)->_userpointer;
	if(typetag != 0) {
		rabbit::Class *cl = _instance(o)->_class;
		do{
			if(cl->_typetag == typetag)
				return SQ_OK;
			cl = cl->_base;
		}while(cl != NULL);
		return sq_throwerror(v,_SC("invalid type tag"));
	}
	return SQ_OK;
}

int64_t sq_gettop(rabbit::VirtualMachine* v)
{
	return (v->_top) - v->_stackbase;
}

void sq_settop(rabbit::VirtualMachine* v, int64_t newtop)
{
	int64_t top = sq_gettop(v);
	if(top > newtop)
		sq_pop(v, top - newtop);
	else
		while(top++ < newtop) sq_pushnull(v);
}

void sq_pop(rabbit::VirtualMachine* v, int64_t nelemstopop)
{
	assert(v->_top >= nelemstopop);
	v->pop(nelemstopop);
}

void sq_poptop(rabbit::VirtualMachine* v)
{
	assert(v->_top >= 1);
	v->pop();
}


void sq_remove(rabbit::VirtualMachine* v, int64_t idx)
{
	v->remove(idx);
}

int64_t sq_cmp(rabbit::VirtualMachine* v)
{
	int64_t res;
	v->objCmp(stack_get(v, -1), stack_get(v, -2),res);
	return res;
}

rabbit::Result sq_newslot(rabbit::VirtualMachine* v, int64_t idx, rabbit::Bool bstatic)
{
	sq_aux_paramscheck(v, 3);
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) == rabbit::OT_TABLE || sq_type(self) == rabbit::OT_CLASS) {
		rabbit::ObjectPtr &key = v->getUp(-2);
		if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, _SC("null is not a valid key"));
		v->newSlot(self, key, v->getUp(-1),bstatic?true:false);
		v->pop(2);
	}
	return SQ_OK;
}

rabbit::Result sq_deleteslot(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval)
{
	sq_aux_paramscheck(v, 2);
	rabbit::ObjectPtr *self;
	_GETSAFE_OBJ(v, idx, rabbit::OT_TABLE,self);
	rabbit::ObjectPtr &key = v->getUp(-1);
	if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, _SC("null is not a valid key"));
	rabbit::ObjectPtr res;
	if(!v->deleteSlot(*self, key, res)){
		v->pop();
		return SQ_ERROR;
	}
	if(pushval) v->getUp(-1) = res;
	else v->pop();
	return SQ_OK;
}

rabbit::Result sq_set(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(v->set(self, v->getUp(-2), v->getUp(-1),DONT_FALL_BACK)) {
		v->pop(2);
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result sq_rawset(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	rabbit::ObjectPtr &key = v->getUp(-2);
	if(sq_type(key) == rabbit::OT_NULL) {
		v->pop(2);
		return sq_throwerror(v, _SC("null key"));
	}
	switch(sq_type(self)) {
	case rabbit::OT_TABLE:
		_table(self)->newSlot(key, v->getUp(-1));
		v->pop(2);
		return SQ_OK;
	break;
	case rabbit::OT_CLASS:
		_class(self)->newSlot(_get_shared_state(v), key, v->getUp(-1),false);
		v->pop(2);
		return SQ_OK;
	break;
	case rabbit::OT_INSTANCE:
		if(_instance(self)->set(key, v->getUp(-1))) {
			v->pop(2);
			return SQ_OK;
		}
	break;
	case rabbit::OT_ARRAY:
		if(v->set(self, key, v->getUp(-1),false)) {
			v->pop(2);
			return SQ_OK;
		}
	break;
	default:
		v->pop(2);
		return sq_throwerror(v, _SC("rawset works only on array/table/class and instance"));
	}
	v->raise_Idxerror(v->getUp(-2));return SQ_ERROR;
}

rabbit::Result sq_newmember(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool bstatic)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) != rabbit::OT_CLASS) return sq_throwerror(v, _SC("new member only works with classes"));
	rabbit::ObjectPtr &key = v->getUp(-3);
	if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, _SC("null key"));
	if(!v->newSlotA(self,key,v->getUp(-2),v->getUp(-1),bstatic?true:false,false)) {
		v->pop(3);
		return SQ_ERROR;
	}
	v->pop(3);
	return SQ_OK;
}

rabbit::Result sq_rawnewmember(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool bstatic)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) != rabbit::OT_CLASS) return sq_throwerror(v, _SC("new member only works with classes"));
	rabbit::ObjectPtr &key = v->getUp(-3);
	if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, _SC("null key"));
	if(!v->newSlotA(self,key,v->getUp(-2),v->getUp(-1),bstatic?true:false,true)) {
		v->pop(3);
		return SQ_ERROR;
	}
	v->pop(3);
	return SQ_OK;
}

rabbit::Result sq_setdelegate(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	rabbit::ObjectPtr &mt = v->getUp(-1);
	rabbit::ObjectType type = sq_type(self);
	switch(type) {
	case rabbit::OT_TABLE:
		if(sq_type(mt) == rabbit::OT_TABLE) {
			if(!_table(self)->setDelegate(_table(mt))) {
				return sq_throwerror(v, _SC("delagate cycle"));
			}
			v->pop();
		}
		else if(sq_type(mt)==rabbit::OT_NULL) {
			_table(self)->setDelegate(NULL); v->pop(); }
		else return sq_aux_invalidtype(v,type);
		break;
	case rabbit::OT_USERDATA:
		if(sq_type(mt)==rabbit::OT_TABLE) {
			_userdata(self)->setDelegate(_table(mt)); v->pop(); }
		else if(sq_type(mt)==rabbit::OT_NULL) {
			_userdata(self)->setDelegate(NULL); v->pop(); }
		else return sq_aux_invalidtype(v, type);
		break;
	default:
			return sq_aux_invalidtype(v, type);
		break;
	}
	return SQ_OK;
}

rabbit::Result sq_rawdeleteslot(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval)
{
	sq_aux_paramscheck(v, 2);
	rabbit::ObjectPtr *self;
	_GETSAFE_OBJ(v, idx, rabbit::OT_TABLE,self);
	rabbit::ObjectPtr &key = v->getUp(-1);
	rabbit::ObjectPtr t;
	if(_table(*self)->get(key,t)) {
		_table(*self)->remove(key);
	}
	if(pushval != 0)
		v->getUp(-1) = t;
	else
		v->pop();
	return SQ_OK;
}

rabbit::Result sq_getdelegate(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	switch(sq_type(self)){
	case rabbit::OT_TABLE:
	case rabbit::OT_USERDATA:
		if(!_delegable(self)->_delegate){
			v->pushNull();
			break;
		}
		v->push(rabbit::ObjectPtr(_delegable(self)->_delegate));
		break;
	default: return sq_throwerror(v,_SC("wrong type")); break;
	}
	return SQ_OK;

}

rabbit::Result sq_get(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	rabbit::ObjectPtr &obj = v->getUp(-1);
	if(v->get(self,obj,obj,false,DONT_FALL_BACK))
		return SQ_OK;
	v->pop();
	return SQ_ERROR;
}

rabbit::Result sq_rawget(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	rabbit::ObjectPtr &obj = v->getUp(-1);
	switch(sq_type(self)) {
	case rabbit::OT_TABLE:
		if(_table(self)->get(obj,obj))
			return SQ_OK;
		break;
	case rabbit::OT_CLASS:
		if(_class(self)->get(obj,obj))
			return SQ_OK;
		break;
	case rabbit::OT_INSTANCE:
		if(_instance(self)->get(obj,obj))
			return SQ_OK;
		break;
	case rabbit::OT_ARRAY:{
		if(sq_isnumeric(obj)){
			if(_array(self)->get(tointeger(obj),obj)) {
				return SQ_OK;
			}
		}
		else {
			v->pop();
			return sq_throwerror(v,_SC("invalid index type for an array"));
		}
				  }
		break;
	default:
		v->pop();
		return sq_throwerror(v,_SC("rawget works only on array/table/instance and class"));
	}
	v->pop();
	return sq_throwerror(v,_SC("the index doesn't exist"));
}

rabbit::Result sq_getstackobj(rabbit::VirtualMachine* v,int64_t idx,rabbit::Object *po)
{
	*po=stack_get(v,idx);
	return SQ_OK;
}

const rabbit::Char *sq_getlocal(rabbit::VirtualMachine* v,uint64_t level,uint64_t idx)
{
	uint64_t cstksize=v->_callsstacksize;
	uint64_t lvl=(cstksize-level)-1;
	int64_t stackbase=v->_stackbase;
	if(lvl<cstksize){
		for(uint64_t i=0;i<level;i++){
			rabbit::VirtualMachine::callInfo &ci=v->_callsstack[(cstksize-i)-1];
			stackbase-=ci._prevstkbase;
		}
		rabbit::VirtualMachine::callInfo &ci=v->_callsstack[lvl];
		if(sq_type(ci._closure)!=rabbit::OT_CLOSURE)
			return NULL;
		SQClosure *c=_closure(ci._closure);
		SQFunctionProto *func=c->_function;
		if(func->_noutervalues > (int64_t)idx) {
			v->push(*_outer(c->_outervalues[idx])->_valptr);
			return _stringval(func->_outervalues[idx]._name);
		}
		idx -= func->_noutervalues;
		return func->getLocal(v,stackbase,idx,(int64_t)(ci._ip-func->_instructions)-1);
	}
	return NULL;
}

void sq_pushobject(rabbit::VirtualMachine* v,rabbit::Object obj)
{
	v->push(rabbit::ObjectPtr(obj));
}

void sq_resetobject(rabbit::Object *po)
{
	po->_unVal.pUserPointer=NULL;po->_type=rabbit::OT_NULL;
}

rabbit::Result sq_throwerror(rabbit::VirtualMachine* v,const rabbit::Char *err)
{
	v->_lasterror=rabbit::String::create(_get_shared_state(v),err);
	return SQ_ERROR;
}

rabbit::Result sq_throwobject(rabbit::VirtualMachine* v)
{
	v->_lasterror = v->getUp(-1);
	v->pop();
	return SQ_ERROR;
}


void sq_reseterror(rabbit::VirtualMachine* v)
{
	v->_lasterror.Null();
}

void sq_getlasterror(rabbit::VirtualMachine* v)
{
	v->push(v->_lasterror);
}

rabbit::Result sq_reservestack(rabbit::VirtualMachine* v,int64_t nsize)
{
	if (((uint64_t)v->_top + nsize) > v->_stack.size()) {
		if(v->_nmetamethodscall) {
			return sq_throwerror(v,_SC("cannot resize stack while in a metamethod"));
		}
		v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
	}
	return SQ_OK;
}

rabbit::Result sq_resume(rabbit::VirtualMachine* v,rabbit::Bool retval,rabbit::Bool raiseerror)
{
	if (sq_type(v->getUp(-1)) == rabbit::OT_GENERATOR)
	{
		v->pushNull(); //retval
		if (!v->execute(v->getUp(-2), 0, v->_top, v->getUp(-1), raiseerror, rabbit::VirtualMachine::ET_RESUME_GENERATOR))
		{v->raise_error(v->_lasterror); return SQ_ERROR;}
		if(!retval)
			v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("only generators can be resumed"));
}

rabbit::Result sq_call(rabbit::VirtualMachine* v,int64_t params,rabbit::Bool retval,rabbit::Bool raiseerror)
{
	rabbit::ObjectPtr res;
	if(v->call(v->getUp(-(params+1)),params,v->_top-params,res,raiseerror?true:false)){

		if(!v->_suspended) {
			v->pop(params);//pop args
		}
		if(retval){
			v->push(res); return SQ_OK;
		}
		return SQ_OK;
	}
	else {
		v->pop(params);
		return SQ_ERROR;
	}
	if(!v->_suspended)
		v->pop(params);
	return sq_throwerror(v,_SC("call failed"));
}

rabbit::Result sq_tailcall(rabbit::VirtualMachine* v, int64_t nparams)
{
	rabbit::ObjectPtr &res = v->getUp(-(nparams + 1));
	if (sq_type(res) != rabbit::OT_CLOSURE) {
		return sq_throwerror(v, _SC("only closure can be tail called"));
	}
	SQClosure *clo = _closure(res);
	if (clo->_function->_bgenerator)
	{
		return sq_throwerror(v, _SC("generators cannot be tail called"));
	}
	
	int64_t stackbase = (v->_top - nparams) - v->_stackbase;
	if (!v->tailcall(clo, stackbase, nparams)) {
		return SQ_ERROR;
	}
	return SQ_TAILCALL_FLAG;
}

rabbit::Result sq_suspendvm(rabbit::VirtualMachine* v)
{
	return v->Suspend();
}

rabbit::Result sq_wakeupvm(rabbit::VirtualMachine* v,rabbit::Bool wakeupret,rabbit::Bool retval,rabbit::Bool raiseerror,rabbit::Bool throwerror)
{
	rabbit::ObjectPtr ret;
	if(!v->_suspended)
		return sq_throwerror(v,_SC("cannot resume a vm that is not running any code"));
	int64_t target = v->_suspended_target;
	if(wakeupret) {
		if(target != -1) {
			v->getAt(v->_stackbase+v->_suspended_target)=v->getUp(-1); //retval
		}
		v->pop();
	} else if(target != -1) { v->getAt(v->_stackbase+v->_suspended_target).Null(); }
	rabbit::ObjectPtr dummy;
	if(!v->execute(dummy,-1,-1,ret,raiseerror,throwerror?rabbit::VirtualMachine::ET_RESUME_THROW_VM : rabbit::VirtualMachine::ET_RESUME_VM)) {
		return SQ_ERROR;
	}
	if(retval) {
		v->push(ret);
	}
	return SQ_OK;
}

void sq_setreleasehook(rabbit::VirtualMachine* v,int64_t idx,SQRELEASEHOOK hook)
{
	rabbit::ObjectPtr &ud=stack_get(v,idx);
	switch(sq_type(ud) ) {
		case rabbit::OT_USERDATA:
			_userdata(ud)->setHook(hook);
			break;
		case rabbit::OT_INSTANCE:
			_instance(ud)->_hook = hook;
			break;
		case rabbit::OT_CLASS:
			_class(ud)->_hook = hook;
			break;
		default:
			return;
	}
}

SQRELEASEHOOK sq_getreleasehook(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &ud=stack_get(v,idx);
	switch(sq_type(ud) ) {
		case rabbit::OT_USERDATA:
			return _userdata(ud)->getHook();
			break;
		case rabbit::OT_INSTANCE:
			return _instance(ud)->_hook;
			break;
		case rabbit::OT_CLASS:
			return _class(ud)->_hook;
			break;
		default:
			return NULL;
	}
}

void sq_setcompilererrorhandler(rabbit::VirtualMachine* v,SQCOMPILERERROR f)
{
	_get_shared_state(v)->_compilererrorhandler = f;
}

rabbit::Result sq_writeclosure(rabbit::VirtualMachine* v,SQWRITEFUNC w,rabbit::UserPointer up)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, -1, rabbit::OT_CLOSURE,o);
	unsigned short tag = SQ_BYTECODE_STREAM_TAG;
	if(_closure(*o)->_function->_noutervalues)
		return sq_throwerror(v,_SC("a closure with free variables bound cannot be serialized"));
	if(w(up,&tag,2) != 2)
		return sq_throwerror(v,_SC("io error"));
	if(!_closure(*o)->save(v,up,w))
		return SQ_ERROR;
	return SQ_OK;
}

rabbit::Result sq_readclosure(rabbit::VirtualMachine* v,SQREADFUNC r,rabbit::UserPointer up)
{
	rabbit::ObjectPtr closure;

	unsigned short tag;
	if(r(up,&tag,2) != 2)
		return sq_throwerror(v,_SC("io error"));
	if(tag != SQ_BYTECODE_STREAM_TAG)
		return sq_throwerror(v,_SC("invalid stream"));
	if(!SQClosure::load(v,up,r,closure))
		return SQ_ERROR;
	v->push(closure);
	return SQ_OK;
}

rabbit::Char *sq_getscratchpad(rabbit::VirtualMachine* v,int64_t minsize)
{
	return _get_shared_state(v)->getScratchPad(minsize);
}

rabbit::Result sq_resurrectunreachable(rabbit::VirtualMachine* v)
{
	return sq_throwerror(v,_SC("sq_resurrectunreachable requires a garbage collector build"));
}

// TODO: remove this...
int64_t sq_collectgarbage(rabbit::VirtualMachine* v)
{
	return -1;
}

rabbit::Result sq_getcallee(rabbit::VirtualMachine* v)
{
	if(v->_callsstacksize > 1)
	{
		v->push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("no closure in the calls stack"));
}

const rabbit::Char *sq_getfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	const rabbit::Char *name = NULL;
	switch(sq_type(self))
	{
	case rabbit::OT_CLOSURE:{
		SQClosure *clo = _closure(self);
		SQFunctionProto *fp = clo->_function;
		if(((uint64_t)fp->_noutervalues) > nval) {
			v->push(*(_outer(clo->_outervalues[nval])->_valptr));
			SQOuterVar &ov = fp->_outervalues[nval];
			name = _stringval(ov._name);
		}
					}
		break;
	case rabbit::OT_NATIVECLOSURE:{
		SQNativeClosure *clo = _nativeclosure(self);
		if(clo->_noutervalues > nval) {
			v->push(clo->_outervalues[nval]);
			name = _SC("@NATIVE");
		}
						  }
		break;
	default: break; //shutup compiler
	}
	return name;
}

rabbit::Result sq_setfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	switch(sq_type(self))
	{
	case rabbit::OT_CLOSURE:{
		SQFunctionProto *fp = _closure(self)->_function;
		if(((uint64_t)fp->_noutervalues) > nval){
			*(_outer(_closure(self)->_outervalues[nval])->_valptr) = stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
					}
		break;
	case rabbit::OT_NATIVECLOSURE:
		if(_nativeclosure(self)->_noutervalues > nval){
			_nativeclosure(self)->_outervalues[nval] = stack_get(v,-1);
		}
		else return sq_throwerror(v,_SC("invalid free var index"));
		break;
	default:
		return sq_aux_invalidtype(v, sq_type(self));
	}
	v->pop();
	return SQ_OK;
}

rabbit::Result sq_setattributes(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	rabbit::ObjectPtr &key = stack_get(v,-2);
	rabbit::ObjectPtr &val = stack_get(v,-1);
	rabbit::ObjectPtr attrs;
	if(sq_type(key) == rabbit::OT_NULL) {
		attrs = _class(*o)->_attributes;
		_class(*o)->_attributes = val;
		v->pop(2);
		v->push(attrs);
		return SQ_OK;
	}else if(_class(*o)->getAttributes(key,attrs)) {
		_class(*o)->setAttributes(key,val);
		v->pop(2);
		v->push(attrs);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("wrong index"));
}

rabbit::Result sq_getattributes(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	rabbit::ObjectPtr &key = stack_get(v,-1);
	rabbit::ObjectPtr attrs;
	if(sq_type(key) == rabbit::OT_NULL) {
		attrs = _class(*o)->_attributes;
		v->pop();
		v->push(attrs);
		return SQ_OK;
	}
	else if(_class(*o)->getAttributes(key,attrs)) {
		v->pop();
		v->push(attrs);
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("wrong index"));
}

rabbit::Result sq_getmemberhandle(rabbit::VirtualMachine* v,int64_t idx,rabbit::MemberHandle *handle)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	rabbit::ObjectPtr &key = stack_get(v,-1);
	rabbit::Table *m = _class(*o)->_members;
	rabbit::ObjectPtr val;
	if(m->get(key,val)) {
		handle->_static = _isfield(val) ? SQFalse : SQTrue;
		handle->_index = _member_idx(val);
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v,_SC("wrong index"));
}

rabbit::Result _getmemberbyhandle(rabbit::VirtualMachine* v,rabbit::ObjectPtr &self,const rabbit::MemberHandle *handle,rabbit::ObjectPtr *&val)
{
	switch(sq_type(self)) {
		case rabbit::OT_INSTANCE: {
				rabbit::Instance *i = _instance(self);
				if(handle->_static) {
					rabbit::Class *c = i->_class;
					val = &c->_methods[handle->_index].val;
				}
				else {
					val = &i->_values[handle->_index];

				}
			}
			break;
		case rabbit::OT_CLASS: {
				rabbit::Class *c = _class(self);
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

rabbit::Result sq_getbyhandle(rabbit::VirtualMachine* v,int64_t idx,const rabbit::MemberHandle *handle)
{
	rabbit::ObjectPtr &self = stack_get(v,idx);
	rabbit::ObjectPtr *val = NULL;
	if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
		return SQ_ERROR;
	}
	v->push(_realval(*val));
	return SQ_OK;
}

rabbit::Result sq_setbyhandle(rabbit::VirtualMachine* v,int64_t idx,const rabbit::MemberHandle *handle)
{
	rabbit::ObjectPtr &self = stack_get(v,idx);
	rabbit::ObjectPtr &newval = stack_get(v,-1);
	rabbit::ObjectPtr *val = NULL;
	if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
		return SQ_ERROR;
	}
	*val = newval;
	v->pop();
	return SQ_OK;
}

rabbit::Result sq_getbase(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	if(_class(*o)->_base)
		v->push(rabbit::ObjectPtr(_class(*o)->_base));
	else
		v->pushNull();
	return SQ_OK;
}

rabbit::Result sq_getclass(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_INSTANCE,o);
	v->push(rabbit::ObjectPtr(_instance(*o)->_class));
	return SQ_OK;
}

rabbit::Result sq_createinstance(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	v->push(_class(*o)->createInstance());
	return SQ_OK;
}

void sq_weakref(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::Object &o=stack_get(v,idx);
	if(ISREFCOUNTED(sq_type(o))) {
		v->push(_refcounted(o)->getWeakRef(sq_type(o)));
		return;
	}
	v->push(o);
}

rabbit::Result sq_getweakrefval(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_WEAKREF) {
		return sq_throwerror(v,_SC("the object must be a weakref"));
	}
	v->push(_weakref(o)->_obj);
	return SQ_OK;
}

rabbit::Result sq_getdefaultdelegate(rabbit::VirtualMachine* v,rabbit::ObjectType t)
{
	rabbit::SharedState *ss = _get_shared_state(v);
	switch(t) {
	case rabbit::OT_TABLE: v->push(ss->_table_default_delegate); break;
	case rabbit::OT_ARRAY: v->push(ss->_array_default_delegate); break;
	case rabbit::OT_STRING: v->push(ss->_string_default_delegate); break;
	case rabbit::OT_INTEGER: case rabbit::OT_FLOAT: v->push(ss->_number_default_delegate); break;
	case rabbit::OT_GENERATOR: v->push(ss->_generator_default_delegate); break;
	case rabbit::OT_CLOSURE: case rabbit::OT_NATIVECLOSURE: v->push(ss->_closure_default_delegate); break;
	case rabbit::OT_THREAD: v->push(ss->_thread_default_delegate); break;
	case rabbit::OT_CLASS: v->push(ss->_class_default_delegate); break;
	case rabbit::OT_INSTANCE: v->push(ss->_instance_default_delegate); break;
	case rabbit::OT_WEAKREF: v->push(ss->_weakref_default_delegate); break;
	default: return sq_throwerror(v,_SC("the type doesn't have a default delegate"));
	}
	return SQ_OK;
}

rabbit::Result sq_next(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr o=stack_get(v,idx),&refpos = stack_get(v,-1),realkey,val;
	if(sq_type(o) == rabbit::OT_GENERATOR) {
		return sq_throwerror(v,_SC("cannot iterate a generator"));
	}
	int faketojump;
	if(!v->FOREACH_OP(o,realkey,val,refpos,0,666,faketojump))
		return SQ_ERROR;
	if(faketojump != 666) {
		v->push(realkey);
		v->push(val);
		return SQ_OK;
	}
	return SQ_ERROR;
}

struct BufState{
	const rabbit::Char *buf;
	int64_t ptr;
	int64_t size;
};

int64_t buf_lexfeed(rabbit::UserPointer file)
{
	BufState *buf=(BufState*)file;
	if(buf->size<(buf->ptr+1))
		return 0;
	return buf->buf[buf->ptr++];
}

rabbit::Result sq_compilebuffer(rabbit::VirtualMachine* v,const rabbit::Char *s,int64_t size,const rabbit::Char *sourcename,rabbit::Bool raiseerror) {
	BufState buf;
	buf.buf = s;
	buf.size = size;
	buf.ptr = 0;
	return sq_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void sq_move(rabbit::VirtualMachine* dest,rabbit::VirtualMachine* src,int64_t idx)
{
	dest->push(stack_get(src,idx));
}

void sq_setprintfunc(rabbit::VirtualMachine* v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc)
{
	_get_shared_state(v)->_printfunc = printfunc;
	_get_shared_state(v)->_errorfunc = errfunc;
}

SQPRINTFUNCTION sq_getprintfunc(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_printfunc;
}

SQPRINTFUNCTION sq_geterrorfunc(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_errorfunc;
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
