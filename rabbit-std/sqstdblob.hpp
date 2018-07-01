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
		rabbit::UserPointer createblob(rabbit::VirtualMachine* v, int64_t size);
		rabbit::Result getblob(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *ptr);
		int64_t getblobsize(rabbit::VirtualMachine* v,int64_t idx);
		
		rabbit::Result register_bloblib(rabbit::VirtualMachine* v);

	}
}
