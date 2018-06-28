/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Instance.hpp>

void rabbit::Instance::init(SQSharedState *ss) {
	_userpointer = NULL;
	_hook = NULL;
	__ObjaddRef(_class);
	_delegate = _class->_members;
}

rabbit::Instance::Instance(SQSharedState *ss, rabbit::Class *c, int64_t memsize) {
	_memsize = memsize;
	_class = c;
	uint64_t nvalues = _class->_defaultvalues.size();
	for(uint64_t n = 0; n < nvalues; n++) {
		new (&_values[n]) rabbit::ObjectPtr(_class->_defaultvalues[n].val);
	}
	init(ss);
}

rabbit::Instance::Instance(SQSharedState *ss, rabbit::Instance *i, int64_t memsize) {
	_memsize = memsize;
	_class = i->_class;
	uint64_t nvalues = _class->_defaultvalues.size();
	for(uint64_t n = 0; n < nvalues; n++) {
		new (&_values[n]) rabbit::ObjectPtr(i->_values[n]);
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
	if(sq_type(_class->_metamethods[mm]) != rabbit::OT_NULL) {
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
