/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once
#include <etk/types.hpp>

void *sq_vm_malloc(uint64_t size);
void *sq_vm_realloc(void *p,uint64_t oldsize,uint64_t size);
void sq_vm_free(void *p,uint64_t size);

#define SQ_MALLOC(__size) sq_vm_malloc((__size));
#define SQ_FREE(__ptr,__size) sq_vm_free((__ptr),(__size));
#define SQ_REALLOC(__ptr,__oldsize,__size) sq_vm_realloc((__ptr),(__oldsize),(__size));

#define sq_aligning(v) (((size_t)(v) + (SQ_ALIGNMENT-1)) & (~(SQ_ALIGNMENT-1)))



#define _CONSTRUCT_VECTOR(type, size, ptr) { \
	for(int64_t n = 0; n < ((int64_t)size); n++) { \
			new ((char*)&ptr[n]) type(); \
		} \
}

#define _DESTRUCT_VECTOR(type, size, ptr) { \
	for(int64_t nl = 0; nl < ((int64_t)size); nl++) { \
			ptr[nl].~type(); \
	} \
}

#define _COPY_VECTOR(dest, src, size) { \
	for(int64_t _n_ = 0; _n_ < ((int64_t)size); _n_++) { \
		dest[_n_] = src[_n_]; \
	} \
}

#define _NULL_SQOBJECT_VECTOR(vec, size) { \
	for(int64_t _n_ = 0; _n_ < ((int64_t)size); _n_++) { \
		vec[_n_].Null(); \
	} \
}

