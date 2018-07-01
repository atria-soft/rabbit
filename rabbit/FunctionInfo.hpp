/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/sqconfig.hpp>

namespace rabbit {
	class FunctionInfo {
		public:
			rabbit::UserPointer funcid;
			const char* name;
			const char* source;
			int64_t line;
	};
}
