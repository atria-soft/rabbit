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
			const rabbit::Char* funcname;
			const rabbit::Char* source;
			int64_t line;
	};

}
