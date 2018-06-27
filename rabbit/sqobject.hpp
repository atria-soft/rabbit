/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/squtils.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/WeakRef.hpp>

#ifdef _SQ64
#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)
#else
#define UINT_MINUS_ONE (0xFFFFFFFF)
#endif

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))

struct SQSharedState;

#define _CONSTRUCT_VECTOR(type, size, ptr) { \
	for(int64_t n = 0; n < ((int64_t)size); n++) { \
			new (&ptr[n]) type(); \
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


inline void _Swap(rabbit::Object &a,rabbit::Object &b)
{
	rabbit::ObjectType tOldType = a._type;
	rabbit::ObjectValue unOldVal = a._unVal;
	a._type = b._type;
	a._unVal = b._unVal;
	b._type = tOldType;
	b._unVal = unOldVal;
}