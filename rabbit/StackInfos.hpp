/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>

namespace rabbit {

	class StackInfos{
		public:
			const char* funcname;
			const char* source;
			int64_t line;
	};

}
