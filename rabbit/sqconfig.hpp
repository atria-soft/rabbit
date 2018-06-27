/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>

// should be the same size of a pointer
using SQHash = size_t;


#ifdef SQUSEDOUBLE
typedef double float_t;
#else
typedef float float_t;
#endif

#if defined(SQUSEDOUBLE) && !defined(_SQ64) || !defined(SQUSEDOUBLE) && defined(_SQ64)
typedef uint64_t SQRawObjectVal; //must be 64bits
#define SQ_OBJECT_RAWINIT() { _unVal.raw = 0; }
#else
typedef uint64_t SQRawObjectVal; //is 32 bits on 32 bits builds and 64 bits otherwise
#define SQ_OBJECT_RAWINIT()
#endif

#ifndef SQ_ALIGNMENT
	#define SQ_ALIGNMENT 8
#endif

namespace rabbit {
	using UserPointer = void*;
	using Bool = uint64_t;
	using Result = int64_t;
	
}

#ifdef SQUNICODE
	#include <wchar.h>
	#include <wctype.h>
	namespace rabbit {
		using Char = wchar_t;
	}
	#define scstrcmp	wcscmp
	#ifdef _WIN32
		#define scsprintf   _snwprintf
	#else
		#define scsprintf   swprintf
	#endif
	#define scstrlen	wcslen
	#define scstrtod	wcstod
	#ifdef _SQ64
		#define scstrtol	wcstoll
	#else
		#define scstrtol	wcstol
	#endif
	#define scstrtoul   wcstoul
	#define scvsprintf  vswprintf
	#define scstrstr	wcsstr
	#define scprintf	wprintf

	#ifdef _WIN32
		#define WCHAR_SIZE 2
		#define WCHAR_SHIFT_MUL 1
		#define MAX_CHAR 0xFFFF
	#else
		#define WCHAR_SIZE 4
		#define WCHAR_SHIFT_MUL 2
		#define MAX_CHAR 0xFFFFFFFF
	#endif
	#define _SC(a) L##a
	#define scisspace   iswspace
	#define scisdigit   iswdigit
	#define scisprint   iswprint
	#define scisxdigit  iswxdigit
	#define scisalpha   iswalpha
	#define sciscntrl   iswcntrl
	#define scisalnum   iswalnum
	#define sq_rsl(l) ((l)<<WCHAR_SHIFT_MUL)
#else
	namespace rabbit {
		using Char = char;
	}
	#define _SC(a) a
	#define scstrcmp	strcmp
	#ifdef _MSC_VER
		#define scsprintf   _snprintf
	#else
		#define scsprintf   snprintf
	#endif
	#define scstrlen	strlen
	#define scstrtod	strtod
	#ifdef _SQ64
		#ifdef _MSC_VER
			#define scstrtol	_strtoi64
		#else
			#define scstrtol	strtoll
		#endif
	#else
		#define scstrtol	strtol
	#endif
	#define scstrtoul   strtoul
	#define scvsprintf  vsnprintf
	#define scstrstr	strstr
	#define scisspace   isspace
	#define scisdigit   isdigit
	#define scisprint   isprint
	#define scisxdigit  isxdigit
	#define sciscntrl   iscntrl
	#define scisalpha   isalpha
	#define scisalnum   isalnum
	#define scprintf	printf
	#define MAX_CHAR 0xFF
	
	#define sq_rsl(l) (l)

#endif

#define _PRINT_INT_PREC _SC("ll")
#define _PRINT_INT_FMT _SC("%ld")

#define SQTrue  (1)
#define SQFalse (0)

struct SQTable;
struct SQString;
struct SQClosure;
struct SQGenerator;
struct SQNativeClosure;
struct SQFunctionProto;
struct SQClass;
struct SQInstance;
struct SQOuter;
struct SQSharedState;
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
}