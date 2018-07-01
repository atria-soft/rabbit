/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	namespace std {
		void seterrorhandlers(rabbit::VirtualMachine* v);
		void printcallstack(rabbit::VirtualMachine* v);
	}
}

