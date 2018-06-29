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
	class Instruction;
	class ExceptionTrap {
		public:
			ExceptionTrap() = default;
			ExceptionTrap(int64_t ss,
			              int64_t stackbase,
			              rabbit::Instruction *ip,
			              int64_t ex_target);
			ExceptionTrap(const rabbit::ExceptionTrap &et);
		
			int64_t _stackbase = 0;
			int64_t _stacksize = 0;
			rabbit::Instruction *_ip = nullptr;
			int64_t _extarget = 0;
	};
}
