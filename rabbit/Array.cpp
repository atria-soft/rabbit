/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Array.hpp>



void rabbit::Array::extend(const rabbit::Array *a){
	int64_t xlen = a->size();
	// TODO: check this condition...
	if(xlen) {
		for(int64_t iii=0;iii<xlen;++iii) {
			append((*a)[iii]);
		}
	}
}
