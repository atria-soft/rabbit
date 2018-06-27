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

struct SQInstruction;
namespace rabbit {
	class ExceptionTrap {
		public:
			ExceptionTrap() {
				
			}
			ExceptionTrap(int64_t ss,
			                int64_t stackbase,
			                SQInstruction *ip,
			                int64_t ex_target) {
				_stacksize = ss;
				_stackbase = stackbase;
				_ip = ip;
				_extarget = ex_target;
			}
			ExceptionTrap(const rabbit::ExceptionTrap &et) {
				(*this) = et;
			}
		
			int64_t _stackbase;
			int64_t _stacksize;
			SQInstruction *_ip;
			int64_t _extarget;
	};
}
