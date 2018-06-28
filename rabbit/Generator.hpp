/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	class Generator : public rabbit::RefCounted {
		public:
			enum rabbit::GeneratorState{eRunning,eSuspended,eDead};
		private:
			rabbit::Generator(rabbit::SharedState *ss,rabbit::Closure *closure){
				_closure=closure;
				_state=eRunning;
				_ci._generator=NULL;
			}
		public:
			static rabbit::Generator *create(rabbit::SharedState *ss,rabbit::Closure *closure){
				rabbit::Generator *nc=(rabbit::Generator*)SQ_MALLOC(sizeof(rabbit::Generator));
				new (nc) rabbit::Generator(ss,closure);
				return nc;
			}
			~rabbit::Generator()
			{
				
			}
			void kill(){
				_state=eDead;
				_stack.resize(0);
				_closure.Null();}
			void release(){
				sq_delete(this,rabbit::Generator);
			}
		
			bool yield(rabbit::VirtualMachine *v,int64_t target);
			bool resume(rabbit::VirtualMachine *v,rabbit::ObjectPtr &dest);
			rabbit::ObjectPtr _closure;
			etk::Vector<rabbit::ObjectPtr> _stack;
			rabbit::VirtualMachine::callInfo _ci;
			etk::Vector<rabbit::ExceptionTrap> _etraps;
			rabbit::GeneratorState _state;
	};

}
