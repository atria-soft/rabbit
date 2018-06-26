/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/RefCounted.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/squtils.hpp>
#include <etk/Allocator.hpp>

rabbit::WeakRef * rabbit::RefCounted::getWeakRef(SQObjectType type) {
	if(!_weakref) {
		sq_new(_weakref, WeakRef);
#if defined(SQUSEDOUBLE) && !defined(_SQ64)
		_weakref->_obj._unVal.raw = 0; //clean the whole union on 32 bits with double
#endif
		_weakref->_obj._type = type;
		_weakref->_obj._unVal.pRefCounted = this;
	}
	return _weakref;
}

rabbit::RefCounted::~RefCounted() {
	if(_weakref) {
		_weakref->_obj._type = OT_NULL;
		_weakref->_obj._unVal.pRefCounted = NULL;
	}
}

void rabbit::RefCounted::refCountIncrement() {
	_uiRef++;
}

int64_t rabbit::RefCounted::refCountDecrement() {
	return --_uiRef;
}

int64_t rabbit::RefCounted::refCountget() {
	return _uiRef;
}
