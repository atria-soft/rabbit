/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/sqconfig.hpp>

namespace rabbit {
	
	union ObjectValue {
		struct SQClosure *pClosure;
		struct SQOuter *pOuter;
		struct SQGenerator *pGenerator;
		struct SQNativeClosure *pNativeClosure;
		int64_t nInteger;
		float_t fFloat;
		struct SQFunctionProto *pFunctionProto;
		
		rabbit::Table* pTable;
		rabbit::String* pString;
		rabbit::Class* pClass;
		rabbit::Instance* pInstance;
		rabbit::Delegable* pDelegable;
		rabbit::UserPointer pUserPointer;
		rabbit::WeakRef* pWeakRef;
		rabbit::VirtualMachine* pThread;
		rabbit::RefCounted* pRefCounted;
		rabbit::Array* pArray;
		rabbit::UserData* pUserData;
		
		SQRawObjectVal raw;
	};

}
