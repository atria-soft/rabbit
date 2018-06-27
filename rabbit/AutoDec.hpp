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
	class AutoDec {
		public:
			AutoDec(int64_t* _count);
			~AutoDec();
		private:
			int64_t* m_countPointer;
	};
}

