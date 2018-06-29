/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/ExceptionTrap.hpp>
#include <rabbit/Instruction.hpp>


rabbit::ExceptionTrap::ExceptionTrap(int64_t ss,
                                     int64_t stackbase,
                                     rabbit::Instruction *ip,
                                     int64_t ex_target) {
	_stacksize = ss;
	_stackbase = stackbase;
	_ip = ip;
	_extarget = ex_target;
}

rabbit::ExceptionTrap::ExceptionTrap(const rabbit::ExceptionTrap &et) {
	(*this) = et;
}