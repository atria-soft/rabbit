/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Closure.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/rabbit.hpp>

#include <rabbit/ObjectPtr.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/FunctionProto.hpp>

#include <rabbit/LocalVarInfo.hpp>
#include <rabbit/LineInfo.hpp>
#include <rabbit/OuterVar.hpp>
#include <rabbit/String.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/Class.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/squtils.hpp>



bool rabbit::SafeWrite(rabbit::VirtualMachine* v,SQWRITEFUNC write,rabbit::UserPointer up,rabbit::UserPointer dest,int64_t size)
{
	if(write(up,dest,size) != size) {
		v->raise_error("io error (write function failure)");
		return false;
	}
	return true;
}

bool rabbit::SafeRead(rabbit::VirtualMachine* v,SQWRITEFUNC read,rabbit::UserPointer up,rabbit::UserPointer dest,int64_t size)
{
	if(size && read(up,dest,size) != size) {
		v->raise_error("io error, read function failure, the origin stream could be corrupted/trucated");
		return false;
	}
	return true;
}

bool rabbit::WriteTag(rabbit::VirtualMachine* v,SQWRITEFUNC write,rabbit::UserPointer up,uint32_t tag)
{
	return SafeWrite(v,write,up,&tag,sizeof(tag));
}

bool rabbit::CheckTag(rabbit::VirtualMachine* v,SQWRITEFUNC read,rabbit::UserPointer up,uint32_t tag)
{
	uint32_t t;
	_CHECK_IO(SafeRead(v,read,up,&t,sizeof(t)));
	if(t != tag){
		v->raise_error("invalid or corrupted closure stream");
		return false;
	}
	return true;
}

bool rabbit::WriteObject(rabbit::VirtualMachine* v,rabbit::UserPointer up,SQWRITEFUNC write,rabbit::ObjectPtr &o)
{
	uint32_t _type = (uint32_t)o.getType();
	_CHECK_IO(SafeWrite(v,write,up,&_type,sizeof(_type)));
	switch(o.getType()){
		case rabbit::OT_STRING:
			_CHECK_IO(SafeWrite(v,write,up,&o.toString()->_len,sizeof(int64_t)));
			_CHECK_IO(SafeWrite(v,write,up,o.getStringValue(),sq_rsl(o.toString()->_len)));
			break;
		case rabbit::OT_BOOL:
		case rabbit::OT_INTEGER:
			_CHECK_IO(SafeWrite(v,write,up,&o.toInteger(),sizeof(int64_t)));break;
		case rabbit::OT_FLOAT:
			_CHECK_IO(SafeWrite(v,write,up,&o.toFloat(),sizeof(float_t)));break;
		case rabbit::OT_NULL:
			break;
		default:
			v->raise_error("cannot serialize a %s",getTypeName(o));
			return false;
	}
	return true;
}

bool rabbit::ReadObject(rabbit::VirtualMachine* v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &o)
{
	uint32_t _type;
	_CHECK_IO(SafeRead(v,read,up,&_type,sizeof(_type)));
	rabbit::ObjectType t = (rabbit::ObjectType)_type;
	switch(t){
	case rabbit::OT_STRING:{
		int64_t len;
		_CHECK_IO(SafeRead(v,read,up,&len,sizeof(int64_t)));
		_CHECK_IO(SafeRead(v,read,up,_get_shared_state(v)->getScratchPad(sq_rsl(len)),sq_rsl(len)));
		o=rabbit::String::create(_get_shared_state(v),_get_shared_state(v)->getScratchPad(-1),len);
				   }
		break;
	case rabbit::OT_INTEGER:{
		int64_t i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(int64_t))); o = i; break;
					}
	case rabbit::OT_BOOL:{
		int64_t i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(int64_t))); o._type = rabbit::OT_BOOL; o._unVal.nInteger = i; break;
					}
	case rabbit::OT_FLOAT:{
		float_t f;
		_CHECK_IO(SafeRead(v,read,up,&f,sizeof(float_t))); o = f; break;
				  }
	case rabbit::OT_NULL:
		o.Null();
		break;
	default:
		v->raise_error("cannot serialize a %s",IdType2Name(t));
		return false;
	}
	return true;
}

bool rabbit::Closure::save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write)
{
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_HEAD));
	_CHECK_IO(WriteTag(v,write,up,sizeof(char)));
	_CHECK_IO(WriteTag(v,write,up,sizeof(int64_t)));
	_CHECK_IO(WriteTag(v,write,up,sizeof(float_t)));
	_CHECK_IO(_function->save(v,up,write));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_TAIL));
	return true;
}

bool rabbit::Closure::load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret)
{
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_HEAD));
	_CHECK_IO(CheckTag(v,read,up,sizeof(char)));
	_CHECK_IO(CheckTag(v,read,up,sizeof(int64_t)));
	_CHECK_IO(CheckTag(v,read,up,sizeof(float_t)));
	rabbit::ObjectPtr func;
	_CHECK_IO(rabbit::FunctionProto::load(v,up,read,func));
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_TAIL));
	ret = rabbit::Closure::create(_get_shared_state(v),func.toFunctionProto(),v->_roottable.toTable()->getWeakRef(rabbit::OT_TABLE));
	//FIXME: load an root for this closure
	return true;
}

rabbit::Closure::~Closure() {
	__Objrelease(_root);
	__Objrelease(_env);
	__Objrelease(_base);
}

rabbit::Closure::Closure(rabbit::SharedState *ss,rabbit::FunctionProto *func){
	_function = func;
	__ObjaddRef(_function); _base = NULL;
	_env = NULL;
	_root=NULL;
}
rabbit::Closure *rabbit::Closure::create(rabbit::SharedState *ss,rabbit::FunctionProto *func,rabbit::WeakRef *root){
	int64_t size = _CALC_CLOSURE_SIZE(func);
	rabbit::Closure *nc=(rabbit::Closure*)SQ_MALLOC(size);
	new ((char*)nc) rabbit::Closure(ss,func);
	nc->_outervalues = (rabbit::ObjectPtr *)(nc + 1);
	nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
	nc->_root = root;
	__ObjaddRef(nc->_root);
	_CONSTRUCT_VECTOR(rabbit::ObjectPtr,func->_noutervalues,nc->_outervalues);
	_CONSTRUCT_VECTOR(rabbit::ObjectPtr,func->_ndefaultparams,nc->_defaultparams);
	return nc;
}

void rabbit::Closure::release(){
	rabbit::FunctionProto *f = _function;
	int64_t size = _CALC_CLOSURE_SIZE(f);
	_DESTRUCT_VECTOR(ObjectPtr,f->_noutervalues,_outervalues);
	_DESTRUCT_VECTOR(ObjectPtr,f->_ndefaultparams,_defaultparams);
	__Objrelease(_function);
	this->~Closure();
	sq_vm_free(this,size);
}

void rabbit::Closure::setRoot(rabbit::WeakRef *r) {
	__Objrelease(_root);
	_root = r;
	__ObjaddRef(_root);
}

rabbit::Closure* rabbit::Closure::clone() {
	rabbit::FunctionProto *f = _function;
	rabbit::Closure * ret = rabbit::Closure::create(NULL,f,_root);
	ret->_env = _env;
	if(ret->_env) {
		__ObjaddRef(ret->_env);
	}
	_COPY_VECTOR(ret->_outervalues,_outervalues,f->_noutervalues);
	_COPY_VECTOR(ret->_defaultparams,_defaultparams,f->_ndefaultparams);
	return ret;
}

