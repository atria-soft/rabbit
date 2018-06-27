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

struct SQExceptionTrap {
	SQExceptionTrap() {}
	SQExceptionTrap(int64_t ss, int64_t stackbase,SQInstruction *ip, int64_t ex_target){ _stacksize = ss; _stackbase = stackbase; _ip = ip; _extarget = ex_target;}
	SQExceptionTrap(const SQExceptionTrap &et) { (*this) = et;  }
	int64_t _stackbase;
	int64_t _stacksize;
	SQInstruction *_ip;
	int64_t _extarget;
};

#define _INLINE

namespace rabbit {
	class VirtualMachine : public rabbit::RefCounted
	{
		public:
			struct callInfo{
				SQInstruction *_ip;
				SQObjectPtr *_literals;
				SQObjectPtr _closure;
				SQGenerator *_generator;
				int32_t _etraps;
				int32_t _prevstkbase;
				int32_t _prevtop;
				int32_t _target;
				int32_t _ncalls;
				SQBool _root;
			};
	
		public:
			void DebugHookProxy(int64_t type, const SQChar * sourcename, int64_t line, const SQChar * funcname);
			static void _DebugHookProxy(rabbit::VirtualMachine* v, int64_t type, const SQChar * sourcename, int64_t line, const SQChar * funcname);
			enum ExecutionType {
				ET_CALL,
				ET_RESUME_GENERATOR,
				ET_RESUME_VM,
				ET_RESUME_THROW_VM
			};
			VirtualMachine(SQSharedState *ss);
			~VirtualMachine();
			bool init(VirtualMachine *friendvm, int64_t stacksize);
			bool execute(SQObjectPtr &func, int64_t nargs, int64_t stackbase, SQObjectPtr &outres, SQBool raiseerror, ExecutionType et = ET_CALL);
			//starts a native call return when the NATIVE closure returns
			bool callNative(SQNativeClosure *nclosure, int64_t nargs, int64_t newbase, SQObjectPtr &retval, int32_t target, bool &suspend,bool &tailcall);
			bool tailcall(SQClosure *closure, int64_t firstparam, int64_t nparams);
			//starts a RABBIT call in the same "Execution loop"
			bool startcall(SQClosure *closure, int64_t target, int64_t nargs, int64_t stackbase, bool tailcall);
			bool createClassInstance(SQClass *theclass, SQObjectPtr &inst, SQObjectPtr &constructor);
			//call a generic closure pure RABBIT or NATIVE
			bool call(SQObjectPtr &closure, int64_t nparams, int64_t stackbase, SQObjectPtr &outres,SQBool raiseerror);
			SQRESULT Suspend();
		
			void callDebugHook(int64_t type,int64_t forcedline=0);
			void callerrorHandler(SQObjectPtr &e);
			bool get(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, uint64_t getflags, int64_t selfidx);
			int64_t fallBackGet(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
			bool invokeDefaultDelegate(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
			bool set(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val, int64_t selfidx);
			int64_t fallBackSet(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val);
			bool newSlot(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val,bool bstatic);
			bool newSlotA(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,const SQObjectPtr &attrs,bool bstatic,bool raw);
			bool deleteSlot(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &res);
			bool clone(const SQObjectPtr &self, SQObjectPtr &target);
			bool objCmp(const SQObjectPtr &o1, const SQObjectPtr &o2,int64_t &res);
			bool stringCat(const SQObjectPtr &str, const SQObjectPtr &obj, SQObjectPtr &dest);
			static bool isEqual(const SQObjectPtr &o1,const SQObjectPtr &o2,bool &res);
			bool toString(const SQObjectPtr &o,SQObjectPtr &res);
			SQString *printObjVal(const SQObjectPtr &o);
		
		
			void raise_error(const SQChar *s, ...);
			void raise_error(const SQObjectPtr &desc);
			void raise_Idxerror(const SQObjectPtr &o);
			void raise_Compareerror(const SQObject &o1, const SQObject &o2);
			void raise_ParamTypeerror(int64_t nparam,int64_t typemask,int64_t type);
		
			void findOuter(SQObjectPtr &target, SQObjectPtr *stackindex);
			void relocateOuters();
			void closeOuters(SQObjectPtr *stackindex);
		
			bool typeOf(const SQObjectPtr &obj1, SQObjectPtr &dest);
			bool callMetaMethod(SQObjectPtr &closure, SQMetaMethod mm, int64_t nparams, SQObjectPtr &outres);
			bool arithMetaMethod(int64_t op, const SQObjectPtr &o1, const SQObjectPtr &o2, SQObjectPtr &dest);
			bool Return(int64_t _arg0, int64_t _arg1, SQObjectPtr &retval);
			//new stuff
			bool ARITH_OP(uint64_t op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2);
			bool BW_OP(uint64_t op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2);
			bool NEG_OP(SQObjectPtr &trg,const SQObjectPtr &o1);
			bool CMP_OP(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &res);
			bool CLOSURE_OP(SQObjectPtr &target, SQFunctionProto *func);
			bool CLASS_OP(SQObjectPtr &target,int64_t base,int64_t attrs);
			//return true if the loop is finished
			bool FOREACH_OP(SQObjectPtr &o1,SQObjectPtr &o2,SQObjectPtr &o3,SQObjectPtr &o4,int64_t arg_2,int exitpos,int &jump);
			//bool LOCAL_INC(int64_t op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr);
			bool PLOCAL_INC(int64_t op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr);
			bool derefInc(int64_t op,SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix,int64_t arg0);
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
		
			static bool IsFalse(SQObjectPtr &o);
		
			void pop();
			void pop(int64_t n);
			void push(const SQObjectPtr &o);
			void pushNull();
			SQObjectPtr& top();
			SQObjectPtr& popGet();
			SQObjectPtr& getUp(int64_t n);
			SQObjectPtr& getAt(int64_t n);
		
			SQObjectPtrVec _stack;
		
			int64_t _top;
			int64_t _stackbase;
			SQOuter* _openouters;
			SQObjectPtr _roottable;
			SQObjectPtr _lasterror;
			SQObjectPtr _errorhandler;
		
			bool _debughook;
			SQDEBUGHOOK _debughook_native;
			SQObjectPtr _debughook_closure;
		
			SQObjectPtr temp_reg;
		
		
			callInfo* _callsstack;
			int64_t _callsstacksize;
			int64_t _alloccallsstacksize;
			etk::Vector<callInfo> _callstackdata;
		
			etk::Vector<SQExceptionTrap> _etraps;
			callInfo *ci;
			SQUserPointer _foreignptr;
			//VMs sharing the same state
			SQSharedState *_sharedstate;
			int64_t _nnativecalls;
			int64_t _nmetamethodscall;
			SQRELEASEHOOK _releasehook;
			//suspend infos
			SQBool _suspended;
			SQBool _suspended_root;
			int64_t _suspended_target;
			int64_t _suspended_traps;
	};
	
	
	inline SQObjectPtr &stack_get(rabbit::VirtualMachine* _vm,int64_t _idx) {
		if (_idx>=0) {
			return _vm->getAt(_idx+_vm->_stackbase-1);
		}
		return _vm->getUp(_idx);
	}
}

#define _ss(_vm_) (_vm_)->_sharedstate

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
