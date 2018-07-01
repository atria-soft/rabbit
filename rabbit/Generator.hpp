/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/LocalVarInfo.hpp>
#include <rabbit/LineInfo.hpp>
#include <rabbit/OuterVar.hpp>
#include <rabbit/Instruction.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/rabbit.hpp>
#include <etk/Vector.hpp>
#include <rabbit/VirtualMachine.hpp>

namespace rabbit {
	class Generator : public rabbit::RefCounted {
		public:
			enum GeneratorState{eRunning,eSuspended,eDead};
		private:
			Generator(rabbit::SharedState *ss,rabbit::Closure *closure);
		public:
			static Generator *create(rabbit::SharedState *ss,rabbit::Closure *closure);
			~Generator();
			void kill();
			void release();
		
			bool yield(rabbit::VirtualMachine *v,int64_t target);
			bool resume(rabbit::VirtualMachine *v,rabbit::ObjectPtr &dest);
			rabbit::ObjectPtr _closure;
			etk::Vector<rabbit::ObjectPtr> _stack;
			rabbit::VirtualMachine::callInfo _ci;
			etk::Vector<rabbit::ExceptionTrap> _etraps;
			GeneratorState _state;
	};

}
