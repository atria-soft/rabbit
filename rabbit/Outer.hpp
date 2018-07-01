/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/ObjectPtr.hpp>


namespace rabbit {
	class SharedState;
	class Outer : public rabbit::RefCounted {
		private:
			Outer(rabbit::SharedState *ss, rabbit::ObjectPtr *outer);
		public:
			static rabbit::Outer *create(rabbit::SharedState *ss, rabbit::ObjectPtr *outer);
			~Outer();
			void release();
			rabbit::ObjectPtr *_valptr; /* pointer to value on stack, or _value below */
			int64_t	_idx; /* idx in stack array, for relocation */
			rabbit::ObjectPtr _value; /* value of outer after stack frame is closed */
			rabbit::Outer *_next; /* pointer to next outer when frame is open */
	};

}
