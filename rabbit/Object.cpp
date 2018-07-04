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
