/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <etk/Vector.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/ObjectPtr.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/sqconfig.hpp>

namespace rabbit {
	class ObjectPtr;
	class SharedState;
	class WeakRef;
	#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(rabbit::NativeClosure) + (noutervalues*sizeof(rabbit::ObjectPtr)))
	class NativeClosure : public rabbit::RefCounted {
		private:
			NativeClosure(rabbit::SharedState *ss,SQFUNCTION func);
		public:
			static rabbit::NativeClosure *create(rabbit::SharedState *ss,SQFUNCTION func,int64_t nouters);
			rabbit::NativeClosure *clone();
			~NativeClosure();
			void release();
		
			int64_t _nparamscheck;
			etk::Vector<int64_t> _typecheck;
			rabbit::ObjectPtr *_outervalues;
			uint64_t _noutervalues;
			rabbit::WeakRef *_env;
			SQFUNCTION _function;
			rabbit::ObjectPtr _name;
	};

}

