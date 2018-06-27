/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/Delegable.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/sqstate.hpp>

bool rabbit::Delegable::getMetaMethod(rabbit::VirtualMachine *v,rabbit::MetaMethod mm,rabbit::ObjectPtr &res) {
	if(_delegate) {
		return _delegate->get((*_get_shared_state(v)->_metamethods)[mm],res);
	}
	return false;
}

bool rabbit::Delegable::setDelegate(SQTable *mt) {
	SQTable *temp = mt;
	if(temp == this) {
		return false;
	}
	while (temp) {
		if (temp->_delegate == this) {
			//cycle detected
			return false;
		}
		temp = temp->_delegate;
	}
	if (mt) {
		__ObjaddRef(mt);
	}
	__Objrelease(_delegate);
	_delegate = mt;
	return true;
}

