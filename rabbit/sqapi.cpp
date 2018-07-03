/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/VirtualMachine.hpp>


#include <rabbit/Array.hpp>


#include <rabbit/UserData.hpp>
#include <rabbit/Compiler.hpp>
#include <rabbit/FuncState.hpp>
#include <rabbit/Instance.hpp>

#include <rabbit/MemberHandle.hpp>

#include <rabbit/String.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/Generator.hpp>
#include <rabbit/Class.hpp>
#include <rabbit/FunctionProto.hpp>
#include <rabbit/NativeClosure.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/Outer.hpp>
#include <rabbit/Closure.hpp>

static bool sq_aux_gettypedarg(rabbit::VirtualMachine* v,int64_t idx,rabbit::ObjectType type,rabbit::ObjectPtr **o)
{
	*o = &stack_get(v,idx);
	if(sq_type(**o) != type){
		rabbit::ObjectPtr oval = v->printObjVal(**o);
		v->raise_error("wrong argument type, expected '%s' got '%.50s'",IdType2Name(type),_stringval(oval));
		return false;
	}
	return true;
}

#define _GETSAFE_OBJ(v,idx,type,o) { if(!sq_aux_gettypedarg(v,idx,type,&o)) return SQ_ERROR; }

#define sq_aux_paramscheck(v,count) \
{ \
	if(sq_gettop(v) < count){ v->raise_error("not enough params in the stack"); return SQ_ERROR; }\
}

namespace rabbit {
	int64_t sq_aux_invalidtype(rabbit::VirtualMachine* v,rabbit::ObjectType type)
	{
		uint64_t buf_size = 100 *sizeof(char);
		snprintf(_get_shared_state(v)->getScratchPad(buf_size), buf_size, "unexpected type %s", IdType2Name(type));
		return sq_throwerror(v, _get_shared_state(v)->getScratchPad(-1));
	}
}
rabbit::VirtualMachine* rabbit::sq_open(int64_t initialstacksize)
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

rabbit::VirtualMachine* rabbit::sq_newthread(rabbit::VirtualMachine* friendvm, int64_t initialstacksize)
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

int64_t rabbit::sq_getvmstate(rabbit::VirtualMachine* v)
{
	if(v->_suspended)
		return SQ_VMSTATE_SUSPENDED;
	else {
		if(v->_callsstacksize != 0) return SQ_VMSTATE_RUNNING;
		else return SQ_VMSTATE_IDLE;
	}
}

void rabbit::sq_seterrorhandler(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v, -1);
	if(    o.isClosure() == true
	    || o.isNativeClosure() == true
	    || o.isNull() == true) {
		v->_errorhandler = o;
		v->pop();
	}
}

void rabbit::sq_setnativedebughook(rabbit::VirtualMachine* v,SQDEBUGHOOK hook)
{
	v->_debughook_native = hook;
	v->_debughook_closure.Null();
	v->_debughook = hook?true:false;
}

void rabbit::sq_setdebughook(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v,-1);
	if (    o.isClosure() == true
	     || o.isNativeClosure() == true
	     || o.isNull() == true) {
		v->_debughook_closure = o;
		v->_debughook_native = NULL;
		v->_debughook = ! o.isNull();
		v->pop();
	}
}

void rabbit::sq_close(rabbit::VirtualMachine* v)
{
	rabbit::SharedState *ss = _get_shared_state(v);
	ss->_root_vm.toVirtualMachine()->finalize();
	sq_delete(ss, SharedState);
}

int64_t rabbit::sq_getversion()
{
	return RABBIT_VERSION_NUMBER;
}

rabbit::Result rabbit::sq_compile(rabbit::VirtualMachine* v,SQLEXREADFUNC read,rabbit::UserPointer p,const char *sourcename,rabbit::Bool raiseerror)
{
	rabbit::ObjectPtr o;
#ifndef NO_COMPILER
	if(compile(v, read, p, sourcename, o, raiseerror?true:false, _get_shared_state(v)->_debuginfo)) {
		v->push(rabbit::Closure::create(_get_shared_state(v), o.toFunctionProto(), v->_roottable.toTable()->getWeakRef(rabbit::OT_TABLE)));
		return SQ_OK;
	}
	return SQ_ERROR;
#else
	return sq_throwerror(v,"this is a no compiler build");
#endif
}

void rabbit::sq_enabledebuginfo(rabbit::VirtualMachine* v, rabbit::Bool enable)
{
	_get_shared_state(v)->_debuginfo = enable?true:false;
}

void rabbit::sq_notifyallexceptions(rabbit::VirtualMachine* v, rabbit::Bool enable)
{
	_get_shared_state(v)->_notifyallexceptions = enable?true:false;
}

void rabbit::sq_addref(rabbit::VirtualMachine* v,rabbit::Object *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return;
	__addRef(po->_type,po->_unVal);
}

uint64_t rabbit::sq_getrefcount(rabbit::VirtualMachine* v,rabbit::Object *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return 0;
   return po->_unVal.pRefCounted->refCountget();
}

rabbit::Bool rabbit::sq_release(rabbit::VirtualMachine* v,rabbit::Object *po)
{
	if(!ISREFCOUNTED(sq_type(*po))) return SQTrue;
	bool ret = (po->_unVal.pRefCounted->refCountget() <= 1) ? SQTrue : SQFalse;
	__release(po->_type,po->_unVal);
	return ret; //the ret val doesn't work(and cannot be fixed)
}

uint64_t rabbit::sq_getvmrefcount(rabbit::VirtualMachine* SQ_UNUSED_ARG(v), const rabbit::Object *po)
{
	if (!ISREFCOUNTED(sq_type(*po))) return 0;
	return po->_unVal.pRefCounted->refCountget();
}

const char * rabbit::sq_objtostring(const rabbit::Object *o)
{
	if(sq_type(*o) == rabbit::OT_STRING) {
		return _stringval(*o);
	}
	return NULL;
}

int64_t rabbit::sq_objtointeger(const rabbit::Object *o)
{
	if(o->isNumeric() == true) {
		return tointeger(*o);
	}
	return 0;
}

float_t rabbit::sq_objtofloat(const rabbit::Object *o)
{
	if(o->isNumeric() == true) {
		return tofloat(*o);
	}
	return 0;
}

rabbit::Bool rabbit::sq_objtobool(const rabbit::Object *o)
{
	if(o->isBoolean() == true) {
		return o->toInteger();
	}
	return SQFalse;
}

rabbit::UserPointer rabbit::sq_objtouserpointer(const rabbit::Object *o)
{
	if(o->isUserPointer() == true) {
		return o->toUserPointer();
	}
	return 0;
}

void rabbit::sq_pushnull(rabbit::VirtualMachine* v)
{
	v->pushNull();
}

void rabbit::sq_pushstring(rabbit::VirtualMachine* v,const char *s,int64_t len)
{
	if(s)
		v->push(rabbit::ObjectPtr(rabbit::String::create(_get_shared_state(v), s, len)));
	else v->pushNull();
}

void rabbit::sq_pushinteger(rabbit::VirtualMachine* v,int64_t n)
{
	v->push(n);
}

void rabbit::sq_pushbool(rabbit::VirtualMachine* v,rabbit::Bool b)
{
	v->push(b?true:false);
}

void rabbit::sq_pushfloat(rabbit::VirtualMachine* v,float_t n)
{
	v->push(n);
}

void rabbit::sq_pushuserpointer(rabbit::VirtualMachine* v,rabbit::UserPointer p)
{
	v->push(p);
}

void rabbit::sq_pushthread(rabbit::VirtualMachine* v, rabbit::VirtualMachine* thread)
{
	v->push(thread);
}

rabbit::UserPointer rabbit::sq_newuserdata(rabbit::VirtualMachine* v,uint64_t size)
{
	rabbit::UserData *ud = rabbit::UserData::create(_get_shared_state(v), size + SQ_ALIGNMENT);
	v->push(ud);
	return (rabbit::UserPointer)sq_aligning(ud + 1);
}

void rabbit::sq_newtable(rabbit::VirtualMachine* v)
{
	v->push(rabbit::Table::create(_get_shared_state(v), 0));
}

void rabbit::sq_newtableex(rabbit::VirtualMachine* v,int64_t initialcapacity)
{
	v->push(rabbit::Table::create(_get_shared_state(v), initialcapacity));
}

void rabbit::sq_newarray(rabbit::VirtualMachine* v,int64_t size)
{
	v->push(rabbit::Array::create(_get_shared_state(v), size));
}

rabbit::Result rabbit::sq_newclass(rabbit::VirtualMachine* v,rabbit::Bool hasbase)
{
	rabbit::Class *baseclass = NULL;
	if(hasbase) {
		rabbit::ObjectPtr &base = stack_get(v,-1);
		if(sq_type(base) != rabbit::OT_CLASS)
			return sq_throwerror(v,"invalid base type");
		baseclass = base.toClass();
	}
	rabbit::Class *newclass = rabbit::Class::create(_get_shared_state(v), baseclass);
	if(baseclass) v->pop();
	v->push(newclass);
	return SQ_OK;
}

rabbit::Bool rabbit::sq_instanceof(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &inst = stack_get(v,-1);
	rabbit::ObjectPtr &cl = stack_get(v,-2);
	if(    sq_type(inst) != rabbit::OT_INSTANCE
	    || sq_type(cl) != rabbit::OT_CLASS)
		return sq_throwerror(v,"invalid param type");
	return inst.toInstance()->instanceOf(cl.toClass())?SQTrue:SQFalse;
}

rabbit::Result rabbit::sq_arrayappend(rabbit::VirtualMachine* v,int64_t idx)
{
	sq_aux_paramscheck(v,2);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	arr->toArray()->append(v->getUp(-1));
	v->pop();
	return SQ_OK;
}

rabbit::Result rabbit::sq_arraypop(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	if(arr->toArray()->size() > 0) {
		if(pushval != 0){
			v->push(arr->toArray()->top());
		}
		arr->toArray()->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, "empty array");
}

rabbit::Result rabbit::sq_arrayresize(rabbit::VirtualMachine* v,int64_t idx,int64_t newsize)
{
	sq_aux_paramscheck(v,1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	if(newsize >= 0) {
		arr->toArray()->resize(newsize);
		return SQ_OK;
	}
	return sq_throwerror(v,"negative size");
}


rabbit::Result rabbit::sq_arrayreverse(rabbit::VirtualMachine* v,int64_t idx)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *o;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,o);
	rabbit::Array *arr = o->toArray();
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

rabbit::Result rabbit::sq_arrayremove(rabbit::VirtualMachine* v,int64_t idx,int64_t itemidx)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	return arr->toArray()->remove(itemidx) ? SQ_OK : sq_throwerror(v,"index out of range");
}

rabbit::Result rabbit::sq_arrayinsert(rabbit::VirtualMachine* v,int64_t idx,int64_t destpos)
{
	sq_aux_paramscheck(v, 1);
	rabbit::ObjectPtr *arr;
	_GETSAFE_OBJ(v, idx, rabbit::OT_ARRAY,arr);
	rabbit::Result ret = arr->toArray()->insert(destpos, v->getUp(-1)) ? SQ_OK : sq_throwerror(v,"index out of range");
	v->pop();
	return ret;
}

void rabbit::sq_newclosure(rabbit::VirtualMachine* v,SQFUNCTION func,uint64_t nfreevars)
{
	rabbit::NativeClosure *nc = rabbit::NativeClosure::create(_get_shared_state(v), func,nfreevars);
	nc->_nparamscheck = 0;
	for(uint64_t i = 0; i < nfreevars; i++) {
		nc->_outervalues[i] = v->top();
		v->pop();
	}
	v->push(rabbit::ObjectPtr(nc));
}

rabbit::Result rabbit::sq_getclosureinfo(rabbit::VirtualMachine* v,int64_t idx,uint64_t *nparams,uint64_t *nfreevars)
{
	rabbit::Object o = stack_get(v, idx);
	if(sq_type(o) == rabbit::OT_CLOSURE) {
		rabbit::Closure *c = o.toClosure();
		rabbit::FunctionProto *proto = c->_function;
		*nparams = (uint64_t)proto->_nparameters;
		*nfreevars = (uint64_t)proto->_noutervalues;
		return SQ_OK;
	}
	else if(sq_type(o) == rabbit::OT_NATIVECLOSURE)
	{
		rabbit::NativeClosure *c = o.toNativeClosure();
		*nparams = (uint64_t)c->_nparamscheck;
		*nfreevars = c->_noutervalues;
		return SQ_OK;
	}
	return sq_throwerror(v,"the object is not a closure");
}

rabbit::Result rabbit::sq_setnativeclosurename(rabbit::VirtualMachine* v,int64_t idx,const char *name)
{
	rabbit::Object o = stack_get(v, idx);
	if(o.isNativeClosure() == true) {
		rabbit::NativeClosure *nc = o.toNativeClosure();
		nc->_name = rabbit::String::create(_get_shared_state(v),name);
		return SQ_OK;
	}
	return sq_throwerror(v,"the object is not a nativeclosure");
}

rabbit::Result rabbit::sq_setparamscheck(rabbit::VirtualMachine* v,int64_t nparamscheck,const char *typemask)
{
	rabbit::Object o = stack_get(v, -1);
	if(o.isNativeClosure() == false)
		return sq_throwerror(v, "native closure expected");
	rabbit::NativeClosure *nc = o.toNativeClosure();
	nc->_nparamscheck = nparamscheck;
	if(typemask) {
		etk::Vector<int64_t> res;
		if(!compileTypemask(res, typemask))
			return sq_throwerror(v, "invalid typemask");
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

rabbit::Result rabbit::sq_bindenv(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(    o.isNativeClosure() == false
	    && o.isClosure() == false) {
		return sq_throwerror(v, "the target is not a closure");
	}
	rabbit::ObjectPtr &env = stack_get(v,-1);
	if (    env.isTable() == false
	     && env.isArray() ==false
	     && env.isClass() == false
	     && env.isInstance() == false) {
		return sq_throwerror(v,"invalid environment");
	}
	rabbit::WeakRef *w = env.toRefCounted()->getWeakRef(sq_type(env));
	rabbit::ObjectPtr ret;
	if(o.isClosure() == true) {
		rabbit::Closure *c = o.toClosure()->clone();
		__Objrelease(c->_env);
		c->_env = w;
		__ObjaddRef(c->_env);
		if(o.toClosure()->_base) {
			c->_base = o.toClosure()->_base;
			__ObjaddRef(c->_base);
		}
		ret = c;
	}
	else { //then must be a native closure
		rabbit::NativeClosure *c = o.toNativeClosure()->clone();
		__Objrelease(c->_env);
		c->_env = w;
		__ObjaddRef(c->_env);
		ret = c;
	}
	v->pop();
	v->push(ret);
	return SQ_OK;
}

rabbit::Result rabbit::sq_getclosurename(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if (    o.isNativeClosure() == false
	     && o.isClosure() == false) {
		return sq_throwerror(v,"the target is not a closure");
	}
	if (o.isNativeClosure() == true) {
		v->push(o.toNativeClosure()->_name);
	} else {
		//closure
		v->push(o.toClosure()->_function->_name);
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_setclosureroot(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &c = stack_get(v,idx);
	rabbit::Object o = stack_get(v, -1);
	if(c.isClosure() == false) {
		return sq_throwerror(v, "closure expected");
	}
	if(o.isTable() == true) {
		c.toClosure()->setRoot(o.toTable()->getWeakRef(rabbit::OT_TABLE));
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, "invalid type");
}

rabbit::Result rabbit::sq_getclosureroot(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &c = stack_get(v,idx);
	if(c.isClosure() == false) {
		return sq_throwerror(v, "closure expected");
	}
	v->push(c.toClosure()->_root->_obj);
	return SQ_OK;
}

rabbit::Result rabbit::sq_clear(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::Object &o=stack_get(v,idx);
	switch(sq_type(o)) {
		case rabbit::OT_TABLE: o.toTable()->clear();  break;
		case rabbit::OT_ARRAY: o.toArray()->resize(0); break;
		default:
			return sq_throwerror(v, "clear only works on table and array");
		break;

	}
	return SQ_OK;
}

void rabbit::sq_pushroottable(rabbit::VirtualMachine* v)
{
	v->push(v->_roottable);
}

void rabbit::sq_pushregistrytable(rabbit::VirtualMachine* v)
{
	v->push(_get_shared_state(v)->_registry);
}

void rabbit::sq_pushconsttable(rabbit::VirtualMachine* v)
{
	v->push(_get_shared_state(v)->_consts);
}

rabbit::Result rabbit::sq_setroottable(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v, -1);
	if (    o.isTable() == true
	     || o.isNull() == true) {
		v->_roottable = o;
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, "invalid type");
}

rabbit::Result rabbit::sq_setconsttable(rabbit::VirtualMachine* v)
{
	rabbit::Object o = stack_get(v, -1);
	if (o.isTable() == true) {
		_get_shared_state(v)->_consts = o;
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v, "invalid type, expected table");
}

void rabbit::sq_setforeignptr(rabbit::VirtualMachine* v,rabbit::UserPointer p)
{
	v->_foreignptr = p;
}

rabbit::UserPointer rabbit::sq_getforeignptr(rabbit::VirtualMachine* v)
{
	return v->_foreignptr;
}

void rabbit::sq_setsharedforeignptr(rabbit::VirtualMachine* v,rabbit::UserPointer p)
{
	_get_shared_state(v)->_foreignptr = p;
}

rabbit::UserPointer rabbit::sq_getsharedforeignptr(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_foreignptr;
}

void rabbit::sq_setvmreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook)
{
	v->_releasehook = hook;
}

SQRELEASEHOOK rabbit::sq_getvmreleasehook(rabbit::VirtualMachine* v)
{
	return v->_releasehook;
}

void rabbit::sq_setsharedreleasehook(rabbit::VirtualMachine* v,SQRELEASEHOOK hook)
{
	_get_shared_state(v)->_releasehook = hook;
}

SQRELEASEHOOK rabbit::sq_getsharedreleasehook(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_releasehook;
}

void rabbit::sq_push(rabbit::VirtualMachine* v,int64_t idx)
{
	v->push(stack_get(v, idx));
}

rabbit::ObjectType rabbit::sq_gettype(rabbit::VirtualMachine* v,int64_t idx)
{
	return sq_type(stack_get(v, idx));
}

rabbit::Result rabbit::sq_typeof(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	rabbit::ObjectPtr res;
	if(!v->typeOf(o,res)) {
		return SQ_ERROR;
	}
	v->push(res);
	return SQ_OK;
}

rabbit::Result rabbit::sq_tostring(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	rabbit::ObjectPtr res;
	if(!v->toString(o,res)) {
		return SQ_ERROR;
	}
	v->push(res);
	return SQ_OK;
}

void rabbit::sq_tobool(rabbit::VirtualMachine* v, int64_t idx, rabbit::Bool *b)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	*b = rabbit::VirtualMachine::IsFalse(o)?SQFalse:SQTrue;
}

rabbit::Result rabbit::sq_getinteger(rabbit::VirtualMachine* v,int64_t idx,int64_t *i)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	if(o.isNumeric() == true) {
		*i = tointeger(o);
		return SQ_OK;
	}
	if(o.isBoolean() == true) {
		*i = rabbit::VirtualMachine::IsFalse(o)?SQFalse:SQTrue;
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result rabbit::sq_getfloat(rabbit::VirtualMachine* v,int64_t idx,float_t *f)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	if(o.isNumeric() == true) {
		*f = tofloat(o);
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result rabbit::sq_getbool(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool *b)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	if (o.isBoolean() == true) {
		*b = o.toInteger();
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result rabbit::sq_getstringandsize(rabbit::VirtualMachine* v,int64_t idx,const char **c,int64_t *size)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_STRING,o);
	*c = _stringval(*o);
	*size = o->toString()->_len;
	return SQ_OK;
}

rabbit::Result rabbit::sq_getstring(rabbit::VirtualMachine* v,int64_t idx,const char **c)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_STRING,o);
	*c = _stringval(*o);
	return SQ_OK;
}

rabbit::Result rabbit::sq_getthread(rabbit::VirtualMachine* v,int64_t idx,rabbit::VirtualMachine* *thread)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_THREAD,o);
	*thread = o->toVirtualMachine();
	return SQ_OK;
}

rabbit::Result rabbit::sq_clone(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	v->pushNull();
	if(!v->clone(o, stack_get(v, -1))){
		v->pop();
		return SQ_ERROR;
	}
	return SQ_OK;
}

int64_t rabbit::sq_getsize(rabbit::VirtualMachine* v, int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	rabbit::ObjectType type = sq_type(o);
	switch(type) {
	case rabbit::OT_STRING:	 return o.toString()->_len;
	case rabbit::OT_TABLE:	  return o.toTable()->countUsed();
	case rabbit::OT_ARRAY:	  return o.toArray()->size();
	case rabbit::OT_USERDATA:   return o.toUserData()->getsize();
	case rabbit::OT_INSTANCE:   return o.toInstance()->_class->_udsize;
	case rabbit::OT_CLASS:	  return o.toClass()->_udsize;
	default:
		return sq_aux_invalidtype(v, type);
	}
}

rabbit::Hash rabbit::sq_gethash(rabbit::VirtualMachine* v, int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v, idx);
	return HashObj(o);
}

rabbit::Result rabbit::sq_getuserdata(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *p,rabbit::UserPointer *typetag)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_USERDATA,o);
	(*p) = _userdataval(*o);
	if(typetag) {
		*typetag = o->toUserData()->getTypeTag();
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_settypetag(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer typetag)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	switch(sq_type(o)) {
		case rabbit::OT_USERDATA:
			o.toUserData()->setTypeTag(typetag);
			break;
		case rabbit::OT_CLASS:
			o.toClass()->_typetag = typetag;
			break;
		default:
			return sq_throwerror(v,"invalid object type");
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_getobjtypetag(const rabbit::Object *o,rabbit::UserPointer * typetag)
{
  switch(sq_type(*o)) {
	case rabbit::OT_INSTANCE: *typetag = o->toInstance()->_class->_typetag; break;
	case rabbit::OT_USERDATA: *typetag = o->toUserData()->getTypeTag(); break;
	case rabbit::OT_CLASS:	*typetag = o->toClass()->_typetag; break;
	default: return SQ_ERROR;
  }
  return SQ_OK;
}

rabbit::Result rabbit::sq_gettypetag(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *typetag)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if (SQ_FAILED(sq_getobjtypetag(&o, typetag)))
		return SQ_ERROR;// this is not an error it should be a bool but would break backward compatibility
	return SQ_OK;
}

rabbit::Result rabbit::sq_getuserpointer(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer *p)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_USERPOINTER,o);
	(*p) = o->toUserPointer();
	return SQ_OK;
}

rabbit::Result rabbit::sq_setinstanceup(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer p)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_INSTANCE) return sq_throwerror(v,"the object is not a class instance");
	o.toInstance()->_userpointer = p;
	return SQ_OK;
}

rabbit::Result rabbit::sq_setclassudsize(rabbit::VirtualMachine* v, int64_t idx, int64_t udsize)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_CLASS) return sq_throwerror(v,"the object is not a class");
	if(o.toClass()->_locked) return sq_throwerror(v,"the class is locked");
	o.toClass()->_udsize = udsize;
	return SQ_OK;
}


rabbit::Result rabbit::sq_getinstanceup(rabbit::VirtualMachine* v, int64_t idx, rabbit::UserPointer *p,rabbit::UserPointer typetag)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_INSTANCE) return sq_throwerror(v,"the object is not a class instance");
	(*p) = o.toInstance()->_userpointer;
	if(typetag != 0) {
		rabbit::Class *cl = o.toInstance()->_class;
		do{
			if(cl->_typetag == typetag)
				return SQ_OK;
			cl = cl->_base;
		}while(cl != NULL);
		return sq_throwerror(v,"invalid type tag");
	}
	return SQ_OK;
}

int64_t rabbit::sq_gettop(rabbit::VirtualMachine* v)
{
	return (v->_top) - v->_stackbase;
}

void rabbit::sq_settop(rabbit::VirtualMachine* v, int64_t newtop)
{
	int64_t top = sq_gettop(v);
	if(top > newtop)
		sq_pop(v, top - newtop);
	else
		while(top++ < newtop) sq_pushnull(v);
}

void rabbit::sq_pop(rabbit::VirtualMachine* v, int64_t nelemstopop)
{
	assert(v->_top >= nelemstopop);
	v->pop(nelemstopop);
}

void rabbit::sq_poptop(rabbit::VirtualMachine* v)
{
	assert(v->_top >= 1);
	v->pop();
}


void rabbit::sq_remove(rabbit::VirtualMachine* v, int64_t idx)
{
	v->remove(idx);
}

int64_t rabbit::sq_cmp(rabbit::VirtualMachine* v)
{
	int64_t res;
	v->objCmp(stack_get(v, -1), stack_get(v, -2),res);
	return res;
}

rabbit::Result rabbit::sq_newslot(rabbit::VirtualMachine* v, int64_t idx, rabbit::Bool bstatic)
{
	sq_aux_paramscheck(v, 3);
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) == rabbit::OT_TABLE || sq_type(self) == rabbit::OT_CLASS) {
		rabbit::ObjectPtr &key = v->getUp(-2);
		if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, "null is not a valid key");
		v->newSlot(self, key, v->getUp(-1),bstatic?true:false);
		v->pop(2);
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_deleteslot(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval)
{
	sq_aux_paramscheck(v, 2);
	rabbit::ObjectPtr *self;
	_GETSAFE_OBJ(v, idx, rabbit::OT_TABLE,self);
	rabbit::ObjectPtr &key = v->getUp(-1);
	if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, "null is not a valid key");
	rabbit::ObjectPtr res;
	if(!v->deleteSlot(*self, key, res)){
		v->pop();
		return SQ_ERROR;
	}
	if(pushval) v->getUp(-1) = res;
	else v->pop();
	return SQ_OK;
}

rabbit::Result rabbit::sq_set(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(v->set(self, v->getUp(-2), v->getUp(-1),DONT_FALL_BACK)) {
		v->pop(2);
		return SQ_OK;
	}
	return SQ_ERROR;
}

rabbit::Result rabbit::sq_rawset(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	rabbit::ObjectPtr &key = v->getUp(-2);
	if(sq_type(key) == rabbit::OT_NULL) {
		v->pop(2);
		return sq_throwerror(v, "null key");
	}
	switch(sq_type(self)) {
	case rabbit::OT_TABLE:
		self.toTable()->newSlot(key, v->getUp(-1));
		v->pop(2);
		return SQ_OK;
	break;
	case rabbit::OT_CLASS:
		self.toClass()->newSlot(_get_shared_state(v), key, v->getUp(-1),false);
		v->pop(2);
		return SQ_OK;
	break;
	case rabbit::OT_INSTANCE:
		if(self.toInstance()->set(key, v->getUp(-1))) {
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
		return sq_throwerror(v, "rawset works only on array/table/class and instance");
	}
	v->raise_Idxerror(v->getUp(-2));return SQ_ERROR;
}

rabbit::Result rabbit::sq_newmember(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool bstatic)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) != rabbit::OT_CLASS) return sq_throwerror(v, "new member only works with classes");
	rabbit::ObjectPtr &key = v->getUp(-3);
	if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, "null key");
	if(!v->newSlotA(self,key,v->getUp(-2),v->getUp(-1),bstatic?true:false,false)) {
		v->pop(3);
		return SQ_ERROR;
	}
	v->pop(3);
	return SQ_OK;
}

rabbit::Result rabbit::sq_rawnewmember(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool bstatic)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	if(sq_type(self) != rabbit::OT_CLASS) return sq_throwerror(v, "new member only works with classes");
	rabbit::ObjectPtr &key = v->getUp(-3);
	if(sq_type(key) == rabbit::OT_NULL) return sq_throwerror(v, "null key");
	if(!v->newSlotA(self,key,v->getUp(-2),v->getUp(-1),bstatic?true:false,true)) {
		v->pop(3);
		return SQ_ERROR;
	}
	v->pop(3);
	return SQ_OK;
}

rabbit::Result rabbit::sq_setdelegate(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self = stack_get(v, idx);
	rabbit::ObjectPtr &mt = v->getUp(-1);
	rabbit::ObjectType type = sq_type(self);
	switch(type) {
	case rabbit::OT_TABLE:
		if(sq_type(mt) == rabbit::OT_TABLE) {
			if(!self.toTable()->setDelegate(mt.toTable())) {
				return sq_throwerror(v, "delagate cycle");
			}
			v->pop();
		}
		else if(sq_type(mt)==rabbit::OT_NULL) {
			self.toTable()->setDelegate(NULL); v->pop(); }
		else return sq_aux_invalidtype(v,type);
		break;
	case rabbit::OT_USERDATA:
		if(sq_type(mt)==rabbit::OT_TABLE) {
			self.toUserData()->setDelegate(mt.toTable()); v->pop(); }
		else if(sq_type(mt)==rabbit::OT_NULL) {
			self.toUserData()->setDelegate(NULL); v->pop(); }
		else return sq_aux_invalidtype(v, type);
		break;
	default:
			return sq_aux_invalidtype(v, type);
		break;
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_rawdeleteslot(rabbit::VirtualMachine* v,int64_t idx,rabbit::Bool pushval)
{
	sq_aux_paramscheck(v, 2);
	rabbit::ObjectPtr *self;
	_GETSAFE_OBJ(v, idx, rabbit::OT_TABLE,self);
	rabbit::ObjectPtr &key = v->getUp(-1);
	rabbit::ObjectPtr t;
	if(self->toTable()->get(key,t)) {
		self->toTable()->remove(key);
	}
	if(pushval != 0)
		v->getUp(-1) = t;
	else
		v->pop();
	return SQ_OK;
}

rabbit::Result rabbit::sq_getdelegate(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	switch(sq_type(self)){
	case rabbit::OT_TABLE:
	case rabbit::OT_USERDATA:
		if(!self.toDelegable()->_delegate){
			v->pushNull();
			break;
		}
		v->push(rabbit::ObjectPtr(self.toDelegable()->_delegate));
		break;
	default: return sq_throwerror(v,"wrong type"); break;
	}
	return SQ_OK;

}

rabbit::Result rabbit::sq_get(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	rabbit::ObjectPtr &obj = v->getUp(-1);
	if(v->get(self,obj,obj,false,DONT_FALL_BACK))
		return SQ_OK;
	v->pop();
	return SQ_ERROR;
}

rabbit::Result rabbit::sq_rawget(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	rabbit::ObjectPtr &obj = v->getUp(-1);
	switch(sq_type(self)) {
	case rabbit::OT_TABLE:
		if(self.toTable()->get(obj,obj))
			return SQ_OK;
		break;
	case rabbit::OT_CLASS:
		if(self.toClass()->get(obj,obj))
			return SQ_OK;
		break;
	case rabbit::OT_INSTANCE:
		if(self.toInstance()->get(obj,obj))
			return SQ_OK;
		break;
	case rabbit::OT_ARRAY:
		{
			if(obj.isNumeric() == true){
				if(self.toArray()->get(tointeger(obj),obj)) {
					return SQ_OK;
				}
			} else {
				v->pop();
				return sq_throwerror(v,"invalid index type for an array");
			}
		}
		break;
	default:
		v->pop();
		return sq_throwerror(v,"rawget works only on array/table/instance and class");
	}
	v->pop();
	return sq_throwerror(v,"the index doesn't exist");
}

rabbit::Result rabbit::sq_getstackobj(rabbit::VirtualMachine* v,int64_t idx,rabbit::Object *po)
{
	*po=stack_get(v,idx);
	return SQ_OK;
}

const char * rabbit::sq_getlocal(rabbit::VirtualMachine* v,uint64_t level,uint64_t idx)
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
		rabbit::Closure *c=ci._closure.toClosure();
		rabbit::FunctionProto *func=c->_function;
		if(func->_noutervalues > (int64_t)idx) {
			v->push(*c->_outervalues[idx].toOuter()->_valptr);
			return _stringval(func->_outervalues[idx]._name);
		}
		idx -= func->_noutervalues;
		return func->getLocal(v,stackbase,idx,(int64_t)(ci._ip-func->_instructions)-1);
	}
	return NULL;
}

void rabbit::sq_pushobject(rabbit::VirtualMachine* v,rabbit::Object obj)
{
	v->push(rabbit::ObjectPtr(obj));
}

void rabbit::sq_resetobject(rabbit::Object *po)
{
	po->_unVal.pUserPointer=NULL;po->_type=rabbit::OT_NULL;
}

rabbit::Result rabbit::sq_throwerror(rabbit::VirtualMachine* v,const char *err)
{
	v->_lasterror=rabbit::String::create(_get_shared_state(v),err);
	return SQ_ERROR;
}

rabbit::Result rabbit::sq_throwobject(rabbit::VirtualMachine* v)
{
	v->_lasterror = v->getUp(-1);
	v->pop();
	return SQ_ERROR;
}


void rabbit::sq_reseterror(rabbit::VirtualMachine* v)
{
	v->_lasterror.Null();
}

void rabbit::sq_getlasterror(rabbit::VirtualMachine* v)
{
	v->push(v->_lasterror);
}

rabbit::Result rabbit::sq_reservestack(rabbit::VirtualMachine* v,int64_t nsize)
{
	if (((uint64_t)v->_top + nsize) > v->_stack.size()) {
		if(v->_nmetamethodscall) {
			return sq_throwerror(v,"cannot resize stack while in a metamethod");
		}
		v->_stack.resize(v->_stack.size() + ((v->_top + nsize) - v->_stack.size()));
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_resume(rabbit::VirtualMachine* v,rabbit::Bool retval,rabbit::Bool raiseerror)
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
	return sq_throwerror(v,"only generators can be resumed");
}

rabbit::Result rabbit::sq_call(rabbit::VirtualMachine* v,int64_t params,rabbit::Bool retval,rabbit::Bool raiseerror)
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
	return sq_throwerror(v,"call failed");
}

rabbit::Result rabbit::sq_tailcall(rabbit::VirtualMachine* v, int64_t nparams)
{
	rabbit::ObjectPtr &res = v->getUp(-(nparams + 1));
	if (sq_type(res) != rabbit::OT_CLOSURE) {
		return sq_throwerror(v, "only closure can be tail called");
	}
	rabbit::Closure *clo = res.toClosure();
	if (clo->_function->_bgenerator)
	{
		return sq_throwerror(v, "generators cannot be tail called");
	}
	
	int64_t stackbase = (v->_top - nparams) - v->_stackbase;
	if (!v->tailcall(clo, stackbase, nparams)) {
		return SQ_ERROR;
	}
	return SQ_TAILCALL_FLAG;
}

rabbit::Result rabbit::sq_suspendvm(rabbit::VirtualMachine* v)
{
	return v->Suspend();
}

rabbit::Result rabbit::sq_wakeupvm(rabbit::VirtualMachine* v,rabbit::Bool wakeupret,rabbit::Bool retval,rabbit::Bool raiseerror,rabbit::Bool throwerror)
{
	rabbit::ObjectPtr ret;
	if(!v->_suspended)
		return sq_throwerror(v,"cannot resume a vm that is not running any code");
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

void rabbit::sq_setreleasehook(rabbit::VirtualMachine* v,int64_t idx,SQRELEASEHOOK hook)
{
	rabbit::ObjectPtr &ud=stack_get(v,idx);
	switch(sq_type(ud) ) {
		case rabbit::OT_USERDATA:
			ud.toUserData()->setHook(hook);
			break;
		case rabbit::OT_INSTANCE:
			ud.toInstance()->_hook = hook;
			break;
		case rabbit::OT_CLASS:
			ud.toClass()->_hook = hook;
			break;
		default:
			return;
	}
}

SQRELEASEHOOK rabbit::sq_getreleasehook(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &ud=stack_get(v,idx);
	switch(sq_type(ud) ) {
		case rabbit::OT_USERDATA:
			return ud.toUserData()->getHook();
			break;
		case rabbit::OT_INSTANCE:
			return ud.toInstance()->_hook;
			break;
		case rabbit::OT_CLASS:
			return ud.toClass()->_hook;
			break;
		default:
			return NULL;
	}
}

void rabbit::sq_setcompilererrorhandler(rabbit::VirtualMachine* v,SQCOMPILERERROR f)
{
	_get_shared_state(v)->_compilererrorhandler = f;
}

rabbit::Result rabbit::sq_writeclosure(rabbit::VirtualMachine* v,SQWRITEFUNC w,rabbit::UserPointer up)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, -1, rabbit::OT_CLOSURE,o);
	unsigned short tag = SQ_BYTECODE_STREAM_TAG;
	if(o->toClosure()->_function->_noutervalues)
		return sq_throwerror(v,"a closure with free variables bound cannot be serialized");
	if(w(up,&tag,2) != 2)
		return sq_throwerror(v,"io error");
	if(!o->toClosure()->save(v,up,w))
		return SQ_ERROR;
	return SQ_OK;
}

rabbit::Result rabbit::sq_readclosure(rabbit::VirtualMachine* v,SQREADFUNC r,rabbit::UserPointer up)
{
	rabbit::ObjectPtr closure;

	unsigned short tag;
	if(r(up,&tag,2) != 2)
		return sq_throwerror(v,"io error");
	if(tag != SQ_BYTECODE_STREAM_TAG)
		return sq_throwerror(v,"invalid stream");
	if(!rabbit::Closure::load(v,up,r,closure))
		return SQ_ERROR;
	v->push(closure);
	return SQ_OK;
}

char * rabbit::sq_getscratchpad(rabbit::VirtualMachine* v,int64_t minsize)
{
	return _get_shared_state(v)->getScratchPad(minsize);
}

rabbit::Result rabbit::sq_resurrectunreachable(rabbit::VirtualMachine* v)
{
	return sq_throwerror(v,"sq_resurrectunreachable requires a garbage collector build");
}

// TODO: remove this...
int64_t rabbit::sq_collectgarbage(rabbit::VirtualMachine* v)
{
	return -1;
}

rabbit::Result rabbit::sq_getcallee(rabbit::VirtualMachine* v)
{
	if(v->_callsstacksize > 1)
	{
		v->push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return SQ_OK;
	}
	return sq_throwerror(v,"no closure in the calls stack");
}

const char * rabbit::sq_getfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	const char *name = NULL;
	switch(sq_type(self))
	{
	case rabbit::OT_CLOSURE:{
		rabbit::Closure *clo = self.toClosure();
		rabbit::FunctionProto *fp = clo->_function;
		if(((uint64_t)fp->_noutervalues) > nval) {
			v->push(*(clo->_outervalues[nval].toOuter()->_valptr));
			rabbit::OuterVar &ov = fp->_outervalues[nval];
			name = _stringval(ov._name);
		}
					}
		break;
	case rabbit::OT_NATIVECLOSURE:{
		rabbit::NativeClosure *clo = self.toNativeClosure();
		if(clo->_noutervalues > nval) {
			v->push(clo->_outervalues[nval]);
			name = "@NATIVE";
		}
						  }
		break;
	default: break; //shutup compiler
	}
	return name;
}

rabbit::Result rabbit::sq_setfreevariable(rabbit::VirtualMachine* v,int64_t idx,uint64_t nval)
{
	rabbit::ObjectPtr &self=stack_get(v,idx);
	switch(sq_type(self))
	{
	case rabbit::OT_CLOSURE:{
		rabbit::FunctionProto *fp = self.toClosure()->_function;
		if(((uint64_t)fp->_noutervalues) > nval){
			*(self.toClosure()->_outervalues[nval].toOuter()->_valptr) = stack_get(v,-1);
		}
		else return sq_throwerror(v,"invalid free var index");
					}
		break;
	case rabbit::OT_NATIVECLOSURE:
		if(self.toNativeClosure()->_noutervalues > nval){
			self.toNativeClosure()->_outervalues[nval] = stack_get(v,-1);
		}
		else return sq_throwerror(v,"invalid free var index");
		break;
	default:
		return sq_aux_invalidtype(v, sq_type(self));
	}
	v->pop();
	return SQ_OK;
}

rabbit::Result rabbit::sq_setattributes(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	rabbit::ObjectPtr &key = stack_get(v,-2);
	rabbit::ObjectPtr &val = stack_get(v,-1);
	rabbit::ObjectPtr attrs;
	if(sq_type(key) == rabbit::OT_NULL) {
		attrs = o->toClass()->_attributes;
		o->toClass()->_attributes = val;
		v->pop(2);
		v->push(attrs);
		return SQ_OK;
	}else if(o->toClass()->getAttributes(key,attrs)) {
		o->toClass()->setAttributes(key,val);
		v->pop(2);
		v->push(attrs);
		return SQ_OK;
	}
	return sq_throwerror(v,"wrong index");
}

rabbit::Result rabbit::sq_getattributes(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	rabbit::ObjectPtr &key = stack_get(v,-1);
	rabbit::ObjectPtr attrs;
	if(sq_type(key) == rabbit::OT_NULL) {
		attrs = o->toClass()->_attributes;
		v->pop();
		v->push(attrs);
		return SQ_OK;
	}
	else if(o->toClass()->getAttributes(key,attrs)) {
		v->pop();
		v->push(attrs);
		return SQ_OK;
	}
	return sq_throwerror(v,"wrong index");
}

rabbit::Result rabbit::sq_getmemberhandle(rabbit::VirtualMachine* v,int64_t idx,rabbit::MemberHandle *handle)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	rabbit::ObjectPtr &key = stack_get(v,-1);
	rabbit::Table *m = o->toClass()->_members;
	rabbit::ObjectPtr val;
	if(m->get(key,val)) {
		handle->_static = _isfield(val) ? SQFalse : SQTrue;
		handle->_index = _member_idx(val);
		v->pop();
		return SQ_OK;
	}
	return sq_throwerror(v,"wrong index");
}

rabbit::Result _getmemberbyhandle(rabbit::VirtualMachine* v,rabbit::ObjectPtr &self,const rabbit::MemberHandle *handle,rabbit::ObjectPtr *&val)
{
	switch(sq_type(self)) {
		case rabbit::OT_INSTANCE: {
				rabbit::Instance *i = self.toInstance();
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
				rabbit::Class *c = self.toClass();
				if(handle->_static) {
					val = &c->_methods[handle->_index].val;
				}
				else {
					val = &c->_defaultvalues[handle->_index].val;
				}
			}
			break;
		default:
			return sq_throwerror(v,"wrong type(expected class or instance)");
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_getbyhandle(rabbit::VirtualMachine* v,int64_t idx,const rabbit::MemberHandle *handle)
{
	rabbit::ObjectPtr &self = stack_get(v,idx);
	rabbit::ObjectPtr *val = NULL;
	if(SQ_FAILED(_getmemberbyhandle(v,self,handle,val))) {
		return SQ_ERROR;
	}
	v->push(_realval(*val));
	return SQ_OK;
}

rabbit::Result rabbit::sq_setbyhandle(rabbit::VirtualMachine* v,int64_t idx,const rabbit::MemberHandle *handle)
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

rabbit::Result rabbit::sq_getbase(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	if(o->toClass()->_base)
		v->push(rabbit::ObjectPtr(o->toClass()->_base));
	else
		v->pushNull();
	return SQ_OK;
}

rabbit::Result rabbit::sq_getclass(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_INSTANCE,o);
	v->push(rabbit::ObjectPtr(o->toInstance()->_class));
	return SQ_OK;
}

rabbit::Result rabbit::sq_createinstance(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr *o = NULL;
	_GETSAFE_OBJ(v, idx, rabbit::OT_CLASS,o);
	v->push(o->toClass()->createInstance());
	return SQ_OK;
}

void rabbit::sq_weakref(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::Object &o=stack_get(v,idx);
	if(ISREFCOUNTED(sq_type(o))) {
		v->push(o.toRefCounted()->getWeakRef(sq_type(o)));
		return;
	}
	v->push(o);
}

rabbit::Result rabbit::sq_getweakrefval(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr &o = stack_get(v,idx);
	if(sq_type(o) != rabbit::OT_WEAKREF) {
		return sq_throwerror(v,"the object must be a weakref");
	}
	v->push(o.toWeakRef()->_obj);
	return SQ_OK;
}

rabbit::Result rabbit::sq_getdefaultdelegate(rabbit::VirtualMachine* v,rabbit::ObjectType t)
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
	default: return sq_throwerror(v,"the type doesn't have a default delegate");
	}
	return SQ_OK;
}

rabbit::Result rabbit::sq_next(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::ObjectPtr o=stack_get(v,idx),&refpos = stack_get(v,-1),realkey,val;
	if(sq_type(o) == rabbit::OT_GENERATOR) {
		return sq_throwerror(v,"cannot iterate a generator");
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

namespace rabbit {
	struct BufState{
		const char *buf;
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
}
rabbit::Result rabbit::sq_compilebuffer(rabbit::VirtualMachine* v,const char *s,int64_t size,const char *sourcename,rabbit::Bool raiseerror) {
	BufState buf;
	buf.buf = s;
	buf.size = size;
	buf.ptr = 0;
	return sq_compile(v, buf_lexfeed, &buf, sourcename, raiseerror);
}

void rabbit::sq_move(rabbit::VirtualMachine* dest,rabbit::VirtualMachine* src,int64_t idx)
{
	dest->push(stack_get(src,idx));
}

void rabbit::sq_setprintfunc(rabbit::VirtualMachine* v, SQPRINTFUNCTION printfunc,SQPRINTFUNCTION errfunc)
{
	_get_shared_state(v)->_printfunc = printfunc;
	_get_shared_state(v)->_errorfunc = errfunc;
}

SQPRINTFUNCTION rabbit::sq_getprintfunc(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_printfunc;
}

SQPRINTFUNCTION rabbit::sq_geterrorfunc(rabbit::VirtualMachine* v)
{
	return _get_shared_state(v)->_errorfunc;
}

void * rabbit::sq_malloc(uint64_t size)
{
	return SQ_MALLOC(size);
}

void * rabbit::sq_realloc(void* p,uint64_t oldsize,uint64_t newsize)
{
	return SQ_REALLOC(p,oldsize,newsize);
}

void rabbit::sq_free(void *p,uint64_t size)
{
	SQ_FREE(p,size);
}
