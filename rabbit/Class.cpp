/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Class.hpp>
#include <rabbit/Instance.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/squtils.hpp>

#include <rabbit/VirtualMachine.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/Closure.hpp>

rabbit::Class::Class(rabbit::SharedState *ss, rabbit::Class *base) {
	_base = base;
	_typetag = 0;
	_hook = NULL;
	_udsize = 0;
	_locked = false;
	_constructoridx = -1;
	if(_base) {
		_constructoridx = _base->_constructoridx;
		_udsize = _base->_udsize;
		_defaultvalues = base->_defaultvalues;
		_methods = base->_methods;
		_COPY_VECTOR(_metamethods,base->_metamethods, rabbit::MT_LAST);
		__ObjaddRef(_base);
	}
	_members = base?base->_members->clone() : rabbit::Table::create(ss,0);
	__ObjaddRef(_members);
}

void rabbit::Class::finalize() {
	_attributes.Null();
	_NULL_SQOBJECT_VECTOR(_defaultvalues,_defaultvalues.size());
	_methods.resize(0);
	_NULL_SQOBJECT_VECTOR(_metamethods, rabbit::MT_LAST);
	__Objrelease(_members);
	if(_base) {
		__Objrelease(_base);
	}
}

rabbit::Class::~Class() {
	finalize();
}

rabbit::Class* rabbit::Class::create(rabbit::SharedState *ss, Class *base) {
	rabbit::Class *newclass = (Class *)SQ_MALLOC(sizeof(Class));
	new ((char*)newclass) Class(ss, base);
	return newclass;
}

bool rabbit::Class::get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val) {
	if(_members->get(key,val)) {
		if(_isfield(val)) {
			rabbit::ObjectPtr &o = _defaultvalues[_member_idx(val)].val;
			val = _realval(o);
		} else {
			val = _methods[_member_idx(val)].val;
		}
		return true;
	}
	return false;
}

bool rabbit::Class::getConstructor(rabbit::ObjectPtr &ctor) {
	if(_constructoridx != -1) {
		ctor = _methods[_constructoridx].val;
		return true;
	}
	return false;
}

void rabbit::Class::lock() {
	_locked = true;
	if(_base) {
		_base->lock();
	}
}

void rabbit::Class::release() {
	if (_hook) {
		_hook(_typetag,0);
	}
	sq_delete(this, Class);
}

bool rabbit::Class::newSlot(rabbit::SharedState *ss,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,bool bstatic) {
	rabbit::ObjectPtr temp;
	bool belongs_to_static_table =    val.isClosure() == true
	                               || val.isNativeClosure() == true
	                               || bstatic;
	if(_locked && !belongs_to_static_table) {
		//the class already has an instance so cannot be modified
		return false;
	}
	//overrides the default value
	if(_members->get(key,temp) && _isfield(temp)) {
		_defaultvalues[_member_idx(temp)].val = val;
		return true;
	}
	if(belongs_to_static_table) {
		int64_t mmidx;
		if(    (    val.isClosure() == true
		         || val.isNativeClosure() == true )
		    && (mmidx = ss->getMetaMethodIdxByName(key)) != -1) {
			_metamethods[mmidx] = val;
		} else {
			rabbit::ObjectPtr theval = val;
			if(_base && val.isClosure() == true) {
				theval = const_cast<rabbit::Closure*>(val.toClosure())->clone();
				theval.toClosure()->_base = _base;
				__ObjaddRef(_base); //ref for the closure
			}
			if(temp.isNull() == true) {
				bool isconstructor;
				rabbit::VirtualMachine::isEqual(ss->_constructoridx, key, isconstructor);
				if(isconstructor) {
					_constructoridx = (int64_t)_methods.size();
				}
				rabbit::ClassMember m;
				m.val = theval;
				_members->newSlot(key,rabbit::ObjectPtr(_make_method_idx(_methods.size())));
				_methods.pushBack(m);
			} else {
				_methods[_member_idx(temp)].val = theval;
			}
		}
		return true;
	}
	rabbit::ClassMember m;
	m.val = val;
	_members->newSlot(key,rabbit::ObjectPtr(_make_field_idx(_defaultvalues.size())));
	_defaultvalues.pushBack(m);
	return true;
}

rabbit::Instance *rabbit::Class::createInstance() {
	if(!_locked) {
		lock();
	}
	return rabbit::Instance::create(NULL,this);
}

int64_t rabbit::Class::next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval) {
	rabbit::ObjectPtr oval;
	int64_t idx = _members->next(false,refpos,outkey,oval);
	if(idx != -1) {
		if(_ismethod(oval)) {
			outval = _methods[_member_idx(oval)].val;
		} else {
			rabbit::ObjectPtr &o = _defaultvalues[_member_idx(oval)].val;
			outval = _realval(o);
		}
	}
	return idx;
}

bool rabbit::Class::setAttributes(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val) {
	rabbit::ObjectPtr idx;
	if(_members->get(key,idx)) {
		if(_isfield(idx)) {
			_defaultvalues[_member_idx(idx)].attrs = val;
		} else {
			_methods[_member_idx(idx)].attrs = val;
		}
		return true;
	}
	return false;
}

bool rabbit::Class::getAttributes(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &outval) {
	rabbit::ObjectPtr idx;
	if(_members->get(key,idx)) {
		outval = (_isfield(idx)?_defaultvalues[_member_idx(idx)].attrs:_methods[_member_idx(idx)].attrs);
		return true;
	}
	return false;
}


