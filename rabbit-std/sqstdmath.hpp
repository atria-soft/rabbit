/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/rabbit.hpp>
#include <rabbit/RegFunction.hpp>

namespace rabbit {
	namespace std {
		rabbit::Result register_mathlib(rabbit::VirtualMachine* v);
	}
}
