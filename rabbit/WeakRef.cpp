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

void rabbit::WeakRef::release() {
	if(ISREFCOUNTED(_obj._type)) {
		_obj._unVal.pRefCounted->_weakref = null;
	}
	sq_delete(this, WeakRef);
}
