/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	class Outer : public rabbit::RefCounted {
		private:
			rabbit::Outer(rabbit::SharedState *ss, rabbit::ObjectPtr *outer){
				_valptr = outer;
				_next = NULL;
			}
		public:
			static rabbit::Outer *create(rabbit::SharedState *ss, rabbit::ObjectPtr *outer) {
				rabbit::Outer *nc  = (rabbit::Outer*)SQ_MALLOC(sizeof(rabbit::Outer));
				new (nc) rabbit::Outer(ss, outer);
				return nc;
			}
			~Outer() {
				
			}
			void release()
			{
				this->~rabbit::Outer();
				sq_vm_free(this,sizeof(rabbit::Outer));
			}
			rabbit::ObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
			int64_t	_idx;	 /* idx in stack array, for relocation */
			rabbit::ObjectPtr  _value;   /* value of outer after stack frame is closed */
			rabbit::Outer	 *_next;	/* pointer to next outer when frame is open   */
	};

}
