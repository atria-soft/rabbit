/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

void *sq_vm_malloc(uint64_t size);
void *sq_vm_realloc(void *p,uint64_t oldsize,uint64_t size);
void sq_vm_free(void *p,uint64_t size);

#define sq_new(__ptr,__type) {__ptr=(__type *)sq_vm_malloc(sizeof(__type));new ((char*)__ptr) __type;}
#define sq_delete(__ptr,__type) {((__type*)__ptr)->~__type();sq_vm_free(__ptr,sizeof(__type));}

#define SQ_MALLOC(__size) sq_vm_malloc((__size));
#define SQ_FREE(__ptr,__size) sq_vm_free((__ptr),(__size));
#define SQ_REALLOC(__ptr,__oldsize,__size) sq_vm_realloc((__ptr),(__oldsize),(__size));

#define sq_aligning(v) (((size_t)(v) + (SQ_ALIGNMENT-1)) & (~(SQ_ALIGNMENT-1)))


#include <etk/Vector.hpp>

