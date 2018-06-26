/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/rabbit.hpp>

namespace rabbit {
	class WeakRef;
	class RefCounted {
		protected:
			int64_t _uiRef = 0;
			struct WeakRef *_weakref = null;
		public:
			RefCounted() {}
			virtual ~RefCounted();
			WeakRef *getWeakRef(SQObjectType _type);
			virtual void release() = 0;
			void refCountIncrement();
			int64_t refCountDecrement();
			int64_t refCountget();
			friend WeakRef;
	};
}

