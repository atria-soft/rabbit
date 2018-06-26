/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/sqpcheader.hpp>

#ifndef SQ_EXCLUDE_DEFAULT_MEMFUNCTIONS
void *sq_vm_malloc(uint64_t size){ return malloc(size); }

void *sq_vm_realloc(void *p, uint64_t SQ_UNUSED_ARG(oldsize), uint64_t size){ return realloc(p, size); }

void sq_vm_free(void *p, uint64_t SQ_UNUSED_ARG(size)){ free(p); }
#endif
