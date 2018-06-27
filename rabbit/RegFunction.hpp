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
#include <rabbit/rabbit.hpp>

namespace rabbit {

	class RegFunction{
		public:
			const rabbit::Char *name;
			SQFUNCTION f;
			int64_t nparamscheck;
			const rabbit::Char *typemask;
	};

}
