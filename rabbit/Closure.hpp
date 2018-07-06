/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/RefCounted.hpp>
#include <rabbit/sqconfig.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/squtils.hpp>

namespace rabbit {
	class SharedState;
	class FunctionProto;
	class VirtualMachine;
	class WeakRef;
	class ObjectPtr;
	class Class;
	
	
	#define _CALC_CLOSURE_SIZE(func) (sizeof(rabbit::Closure) + (func->_noutervalues*sizeof(rabbit::ObjectPtr)) + (func->_ndefaultparams*sizeof(rabbit::ObjectPtr)))
	
	class Closure : public rabbit::RefCounted {
		private:
			Closure(rabbit::SharedState *ss,rabbit::FunctionProto *func);
		public:
			static Closure *create(rabbit::SharedState *ss,rabbit::FunctionProto *func,rabbit::WeakRef *root);
			void release();
			void setRoot(rabbit::WeakRef *r);
			Closure *clone();
			~Closure();
		
			rabbit::WeakRef *_env;
			rabbit::WeakRef *_root;
			rabbit::Class *_base;
			rabbit::FunctionProto *_function;
			rabbit::ObjectPtr *_outervalues;
			rabbit::ObjectPtr *_defaultparams;
	};

}
#define _CHECK_IO(exp)  { if(!exp)return false; }
#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))
