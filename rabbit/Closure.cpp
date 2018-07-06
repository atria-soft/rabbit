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

