/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/Object.hpp>

namespace rabbit {
	class WeakRef: public RefCounted {
		public:
			void release();
		public:
			rabbit::Object _obj;
		protected:
			friend RefCounted;
	};
}

