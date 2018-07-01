/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <ctype.h>
#ifdef SQUSEDOUBLE
typedef double float_t;
#else
typedef float float_t;
#endif

namespace rabbit {
	#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
		using RawObjectVal = uint64_t; //must be 64bits
		#define SQ_OBJECT_RAWINIT() { _unVal.raw = 0; }
	#else
		using RawObjectVal = uint64_t; //is 32 bits on 32 bits builds and 64 bits otherwise
		#define SQ_OBJECT_RAWINIT()
	#endif
}

#ifndef SQ_ALIGNMENT
	#define SQ_ALIGNMENT 8
#endif

//max number of character for a printed number
#define NUMBER_UINT8_MAX 50

namespace rabbit {
	using UserPointer = void*;
	using Bool = uint64_t;
	using Result = int64_t;
	
}

#define sq_rsl(l) (l)

#define _PRINT_INT_PREC _SC("ll")
#define _PRINT_INT_FMT _SC("%ld")

#define SQTrue  (1)
#define SQFalse (0)


#ifdef _SQ64
#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)
#else
#define UINT_MINUS_ONE (0xFFFFFFFF)
#endif

#define _SC(a) a


namespace rabbit {
	class UserData;
	class Array;
	class RefCounted;
	class WeakRef;
	class VirtualMachine;
	class Delegable;
	class FunctionInfo;
	class StackInfos;
	class MemberHandle;
	class Instance;
	class Class;
	class Table;
	class String;
	class SharedState;
	class Closure;
	class Generator;
	class NativeClosure;
	class FunctionProto;
	class Outer;
}