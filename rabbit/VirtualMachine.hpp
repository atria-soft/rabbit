/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/sqopcodes.hpp>
#include <rabbit/sqobject.hpp>
#include <rabbit/AutoDec.hpp>
#include <rabbit/sqconfig.hpp>
#include <rabbit/ExceptionTrap.hpp>
#include <rabbit/MetaMethod.hpp>
#include <rabbit/ObjectPtr.hpp>


#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

#define SQ_SUSPEND_FLAG -666
#define SQ_TAILCALL_FLAG -777
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW				0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002

//base lib
void sq_base_register(rabbit::VirtualMachine* v);


#define _INLINE

namespace rabbit {
	class VirtualMachine : public rabbit::RefCounted
	{
		public:
			struct callInfo{
				rabbit::Instruction *_ip;
				rabbit::ObjectPtr *_literals;
				rabbit::ObjectPtr _closure;
				rabbit::Generator *_generator;
				int32_t _etraps;
				int32_t _prevstkbase;
				int32_t _prevtop;
				int32_t _target;
				int32_t _ncalls;
				rabbit::Bool _root;
			};
	
		public:
			void DebugHookProxy(int64_t type, const rabbit::Char * sourcename, int64_t line, const rabbit::Char * funcname);
			static void _DebugHookProxy(rabbit::VirtualMachine* v, int64_t type, const rabbit::Char * sourcename, int64_t line, const rabbit::Char * funcname);
			enum ExecutionType {
				ET_CALL,
				ET_RESUME_GENERATOR,
				ET_RESUME_VM,
				ET_RESUME_THROW_VM
			};
			VirtualMachine(rabbit::SharedState *ss);
			~VirtualMachine();
			bool init(VirtualMachine *friendvm, int64_t stacksize);
			bool execute(rabbit::ObjectPtr &func, int64_t nargs, int64_t stackbase, rabbit::ObjectPtr &outres, rabbit::Bool raiseerror, ExecutionType et = ET_CALL);
			//starts a native call return when the NATIVE closure returns
			bool callNative(rabbit::NativeClosure *nclosure, int64_t nargs, int64_t newbase, rabbit::ObjectPtr &retval, int32_t target, bool &suspend,bool &tailcall);
			bool tailcall(rabbit::Closure *closure, int64_t firstparam, int64_t nparams);
			//starts a RABBIT call in the same "Execution loop"
			bool startcall(rabbit::Closure *closure, int64_t target, int64_t nargs, int64_t stackbase, bool tailcall);
			bool createClassInstance(rabbit::Class *theclass, rabbit::ObjectPtr &inst, rabbit::ObjectPtr &constructor);
			//call a generic closure pure RABBIT or NATIVE
			bool call(rabbit::ObjectPtr &closure, int64_t nparams, int64_t stackbase, rabbit::ObjectPtr &outres,rabbit::Bool raiseerror);
			rabbit::Result Suspend();
		
			void callDebugHook(int64_t type,int64_t forcedline=0);
			void callerrorHandler(rabbit::ObjectPtr &e);
			bool get(const rabbit::ObjectPtr &self, const rabbit::ObjectPtr &key, rabbit::ObjectPtr &dest, uint64_t getflags, int64_t selfidx);
			int64_t fallBackGet(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,rabbit::ObjectPtr &dest);
			bool invokeDefaultDelegate(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,rabbit::ObjectPtr &dest);
			bool set(const rabbit::ObjectPtr &self, const rabbit::ObjectPtr &key, const rabbit::ObjectPtr &val, int64_t selfidx);
			int64_t fallBackSet(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
			bool newSlot(const rabbit::ObjectPtr &self, const rabbit::ObjectPtr &key, const rabbit::ObjectPtr &val,bool bstatic);
			bool newSlotA(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,const rabbit::ObjectPtr &attrs,bool bstatic,bool raw);
			bool deleteSlot(const rabbit::ObjectPtr &self, const rabbit::ObjectPtr &key, rabbit::ObjectPtr &res);
			bool clone(const rabbit::ObjectPtr &self, rabbit::ObjectPtr &target);
			bool objCmp(const rabbit::ObjectPtr &o1, const rabbit::ObjectPtr &o2,int64_t &res);
			bool stringCat(const rabbit::ObjectPtr &str, const rabbit::ObjectPtr &obj, rabbit::ObjectPtr &dest);
			static bool isEqual(const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2,bool &res);
			bool toString(const rabbit::ObjectPtr &o,rabbit::ObjectPtr &res);
			rabbit::String *printObjVal(const rabbit::ObjectPtr &o);
		
		
			void raise_error(const rabbit::Char *s, ...);
			void raise_error(const rabbit::ObjectPtr &desc);
			void raise_Idxerror(const rabbit::ObjectPtr &o);
			void raise_Compareerror(const rabbit::Object &o1, const rabbit::Object &o2);
			void raise_ParamTypeerror(int64_t nparam,int64_t typemask,int64_t type);
		
			void findOuter(rabbit::ObjectPtr &target, rabbit::ObjectPtr *stackindex);
			void relocateOuters();
			void closeOuters(rabbit::ObjectPtr *stackindex);
		
			bool typeOf(const rabbit::ObjectPtr &obj1, rabbit::ObjectPtr &dest);
			bool callMetaMethod(rabbit::ObjectPtr &closure, rabbit::MetaMethod mm, int64_t nparams, rabbit::ObjectPtr &outres);
			bool arithMetaMethod(int64_t op, const rabbit::ObjectPtr &o1, const rabbit::ObjectPtr &o2, rabbit::ObjectPtr &dest);
			bool Return(int64_t _arg0, int64_t _arg1, rabbit::ObjectPtr &retval);
			//new stuff
			bool ARITH_OP(uint64_t op,rabbit::ObjectPtr &trg,const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2);
			bool BW_OP(uint64_t op,rabbit::ObjectPtr &trg,const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2);
			bool NEG_OP(rabbit::ObjectPtr &trg,const rabbit::ObjectPtr &o1);
			bool CMP_OP(CmpOP op, const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2,rabbit::ObjectPtr &res);
			bool CLOSURE_OP(rabbit::ObjectPtr &target, rabbit::FunctionProto *func);
			bool CLASS_OP(rabbit::ObjectPtr &target,int64_t base,int64_t attrs);
			//return true if the loop is finished
			bool FOREACH_OP(rabbit::ObjectPtr &o1,rabbit::ObjectPtr &o2,rabbit::ObjectPtr &o3,rabbit::ObjectPtr &o4,int64_t arg_2,int exitpos,int &jump);
			//bool LOCAL_INC(int64_t op,rabbit::ObjectPtr &target, rabbit::ObjectPtr &a, rabbit::ObjectPtr &incr);
			bool PLOCAL_INC(int64_t op,rabbit::ObjectPtr &target, rabbit::ObjectPtr &a, rabbit::ObjectPtr &incr);
			bool derefInc(int64_t op,rabbit::ObjectPtr &target, rabbit::ObjectPtr &self, rabbit::ObjectPtr &key, rabbit::ObjectPtr &incr, bool postfix,int64_t arg0);
		#ifdef _DEBUG_DUMP
			void dumpstack(int64_t stackbase=-1, bool dumpall = false);
		#endif
		
			void finalize();
			void GrowcallStack() {
				int64_t newsize = _alloccallsstacksize*2;
				_callstackdata.resize(newsize);
				_callsstack = &_callstackdata[0];
				_alloccallsstacksize = newsize;
			}
			bool enterFrame(int64_t newbase, int64_t newtop, bool tailcall);
			void leaveFrame();
			void release() {
				sq_delete(this,VirtualMachine);
			}
		////////////////////////////////////////////////////////////////////////////
			//stack functions for the api
			void remove(int64_t n);
		
			static bool IsFalse(rabbit::ObjectPtr &o);
		
			void pop();
			void pop(int64_t n);
			void push(const rabbit::ObjectPtr &o);
			void pushNull();
			rabbit::ObjectPtr& top();
			rabbit::ObjectPtr& popGet();
			rabbit::ObjectPtr& getUp(int64_t n);
			rabbit::ObjectPtr& getAt(int64_t n);
		
			etk::Vector<rabbit::ObjectPtr> _stack;
		
			int64_t _top;
			int64_t _stackbase;
			rabbit::Outer* _openouters;
			rabbit::ObjectPtr _roottable;
			rabbit::ObjectPtr _lasterror;
			rabbit::ObjectPtr _errorhandler;
		
			bool _debughook;
			SQDEBUGHOOK _debughook_native;
			rabbit::ObjectPtr _debughook_closure;
		
			rabbit::ObjectPtr temp_reg;
		
		
			callInfo* _callsstack;
			int64_t _callsstacksize;
			int64_t _alloccallsstacksize;
			etk::Vector<callInfo> _callstackdata;
		
			etk::Vector<rabbit::ExceptionTrap> _etraps;
			callInfo *ci;
			rabbit::UserPointer _foreignptr;
			//VMs sharing the same state
			rabbit::SharedState *_sharedstate;
			int64_t _nnativecalls;
			int64_t _nmetamethodscall;
			SQRELEASEHOOK _releasehook;
			//suspend infos
			rabbit::Bool _suspended;
			rabbit::Bool _suspended_root;
			int64_t _suspended_target;
			int64_t _suspended_traps;
	};
	
	
	inline rabbit::ObjectPtr &stack_get(rabbit::VirtualMachine* _vm,int64_t _idx) {
		if (_idx>=0) {
			return _vm->getAt(_idx+_vm->_stackbase-1);
		}
		return _vm->getUp(_idx);
	}
}

#define _get_shared_state(_vm_) (_vm_)->_sharedstate

#define PUSH_CALLINFO(v,nci){ \
	int64_t css = v->_callsstacksize; \
	if(css == v->_alloccallsstacksize) { \
		v->GrowcallStack(); \
	} \
	v->ci = &v->_callsstack[css]; \
	*(v->ci) = nci; \
	v->_callsstacksize++; \
}

#define POP_CALLINFO(v){ \
	int64_t css = --v->_callsstacksize; \
	v->ci->_closure.Null(); \
	v->ci = css?&v->_callsstack[css-1]:NULL;	\
}
