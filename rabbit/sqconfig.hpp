/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <etk/Allocator.hpp>
#include <ctype.h>
#ifdef SQUSEDOUBLE
typedef double float_t;
#else
typedef float float_t;
#endif

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

#define _PRINT_INT_PREC "ll"
#define _PRINT_INT_FMT "%ld"

#define SQTrue  (1)
#define SQFalse (0)


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