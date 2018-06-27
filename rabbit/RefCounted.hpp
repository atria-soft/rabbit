/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/ObjectType.hpp>

namespace rabbit {
	class WeakRef;
	class RefCounted {
		protected:
			int64_t _uiRef = 0;
			struct WeakRef *_weakref = null;
		public:
			RefCounted() {}
			virtual ~RefCounted();
			WeakRef *getWeakRef(rabbit::ObjectType _type);
			virtual void release() = 0;
			void refCountIncrement();
			int64_t refCountDecrement();
			int64_t refCountget();
			friend WeakRef;
	};
	#define __Objrelease(obj) { \
		if((obj)) { \
			auto val = (obj)->refCountDecrement(); \
			if(val == 0) { \
				(obj)->release(); \
			} \
			(obj) = NULL;   \
		} \
	}
	#define __ObjaddRef(obj) { \
		(obj)->refCountIncrement(); \
	}

}

