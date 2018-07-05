/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/Object.hpp>
#include <rabbit/String.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/UserData.hpp>

const char* rabbit::Object::getStringValue() const {
	return (const char*)&_unVal.pString->_val[0];
}

char* rabbit::Object::getStringValue() {
	return (char*)&_unVal.pString->_val[0];
}


rabbit::Object rabbit::Object::getRealObject() const {
	if (isWeakRef() == false) {
		return *this;
	}
	return toWeakRef()->_obj;
}

rabbit::UserPointer rabbit::Object::getUserDataValue() const {
	return (rabbit::UserPointer)sq_aligning(_unVal.pUserData + 1);
}

void rabbit::Object::addRef() {
	if ((_type&SQOBJECT_REF_COUNTED) != 0) {
		_unVal.pRefCounted->refCountIncrement();
	}
}

void rabbit::Object::releaseRef() {
	if (    (_type&SQOBJECT_REF_COUNTED) != 0
	     && (_unVal.pRefCounted->refCountDecrement()==0)) {
		_unVal.pRefCounted->release();
	}
	_type = rabbit::OT_NULL;
	_unVal.raw = 0;
}

void rabbit::Object::swap(rabbit::Object& _obj) {
	rabbit::ObjectType tOldType = _type;
	rabbit::ObjectValue unOldVal = _unVal;
	_type = _obj._type;
	_unVal = _obj._unVal;
	_obj._type = tOldType;
	_obj._unVal = unOldVal;
}


