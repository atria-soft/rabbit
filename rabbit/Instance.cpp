/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Instance.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/squtils.hpp>
#include <etk/types.hpp>
#include <etk/Allocator.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/Closure.hpp>

#include <rabbit/String.hpp>
#include <rabbit/Class.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/WeakRef.hpp>

void rabbit::Instance::init(rabbit::SharedState *ss) {
	_userpointer = NULL;
	_hook = NULL;
	__ObjaddRef(_class);
	_delegate = _class->_members;
}

rabbit::Instance::Instance(rabbit::SharedState *ss, rabbit::Class *c, int64_t memsize) {
	_memsize = memsize;
	_class = c;
	uint64_t nvalues = _class->_defaultvalues.size();
	for(uint64_t n = 0; n < nvalues; n++) {
		&_values[n] = ETK_NEW(rabbit::ObjectPtr, _class->_defaultvalues[n].val);
	}
	init(ss);
}

rabbit::Instance::Instance(rabbit::SharedState *ss, rabbit::Instance *i, int64_t memsize) {
	_memsize = memsize;
	_class = i->_class;
	uint64_t nvalues = _class->_defaultvalues.size();
	for(uint64_t n = 0; n < nvalues; n++) {
		&_values[n] = ETK_NEW(rabbit::ObjectPtr, i->_values[n]);
	}
	init(ss);
}

void rabbit::Instance::finalize() {
	uint64_t nvalues = _class->_defaultvalues.size();
	__Objrelease(_class);
	_NULL_SQOBJECT_VECTOR(_values,nvalues);
}

rabbit::Instance::~Instance() {
	if(_class){
		finalize();
	}
}

bool rabbit::Instance::getMetaMethod(rabbit::VirtualMachine* SQ_UNUSED_ARG(v),rabbit::MetaMethod mm,rabbit::ObjectPtr &res) {
	if(_class->_metamethods[mm].isNull() == false) {
		res = _class->_metamethods[mm];
		return true;
	}
	return false;
}

bool rabbit::Instance::instanceOf(rabbit::Class *trg) {
	rabbit::Class *parent = _class;
	while(parent != NULL) {
		if(parent == trg) {
			return true;
		}
		parent = parent->_base;
	}
	return false;
}

rabbit::Instance* rabbit::Instance::create(rabbit::SharedState *ss,rabbit::Class *theclass) {
	int64_t size = calcinstancesize(theclass);
	Instance *newinst = ETK_NEW(Instance, ss, theclass,size);
	if(theclass->_udsize) {
		newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
	}
	return newinst;
}

rabbit::Instance* rabbit::Instance::clone(rabbit::SharedState *ss) {
	int64_t size = calcinstancesize(_class);
	Instance *newinst = ETK_NEW(Instance, ss, this, size);
	if(_class->_udsize) {
		newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
	}
	return newinst;
}

bool rabbit::Instance::get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val)  {
	if(_class->_members->get(key,val)) {
		if(_isfield(val)) {
			rabbit::ObjectPtr &o = _values[_member_idx(val)];
			val = o.getRealObject();
		} else {
			val = _class->_methods[_member_idx(val)].val;
		}
		return true;
	}
	return false;
}

bool rabbit::Instance::set(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val) {
	rabbit::ObjectPtr idx;
	if(_class->_members->get(key,idx) && _isfield(idx)) {
		_values[_member_idx(idx)] = val;
		return true;
	}
	return false;
}

void rabbit::Instance::release() {
	_uiRef++;
	if (_hook) {
		_hook(_userpointer,0);
	}
	_uiRef--;
	if(_uiRef > 0) {
		return;
	}
	int64_t size = _memsize;
	ETK_FREE(Instance, this);
}

