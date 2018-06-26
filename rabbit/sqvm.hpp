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

#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 15

#define SQ_SUSPEND_FLAG -666
#define SQ_TAILCALL_FLAG -777
#define DONT_FALL_BACK 666
//#define EXISTS_FALL_BACK -1

#define GET_FLAG_RAW				0x00000001
#define GET_FLAG_DO_NOT_RAISE_ERROR 0x00000002
//base lib
void sq_base_register(HRABBITVM v);

struct SQExceptionTrap{
	SQExceptionTrap() {}
	SQExceptionTrap(int64_t ss, int64_t stackbase,SQInstruction *ip, int64_t ex_target){ _stacksize = ss; _stackbase = stackbase; _ip = ip; _extarget = ex_target;}
	SQExceptionTrap(const SQExceptionTrap &et) { (*this) = et;  }
	int64_t _stackbase;
	int64_t _stacksize;
	SQInstruction *_ip;
	int64_t _extarget;
};

#define _INLINE

typedef sqvector<SQExceptionTrap> ExceptionsTraps;

struct SQVM : public rabbit::RefCounted
{
	struct CallInfo{
		//CallInfo() { _generator = NULL;}
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

typedef sqvector<CallInfo> CallInfoVec;
public:
	void DebugHookProxy(int64_t type, const SQChar * sourcename, int64_t line, const SQChar * funcname);
	static void _DebugHookProxy(HRABBITVM v, int64_t type, const SQChar * sourcename, int64_t line, const SQChar * funcname);
	enum ExecutionType { ET_CALL, ET_RESUME_GENERATOR, ET_RESUME_VM,ET_RESUME_THROW_VM };
	SQVM(SQSharedState *ss);
	~SQVM();
	bool Init(SQVM *friendvm, int64_t stacksize);
	bool Execute(SQObjectPtr &func, int64_t nargs, int64_t stackbase, SQObjectPtr &outres, SQBool raiseerror, ExecutionType et = ET_CALL);
	//starts a native call return when the NATIVE closure returns
	bool CallNative(SQNativeClosure *nclosure, int64_t nargs, int64_t newbase, SQObjectPtr &retval, int32_t target, bool &suspend,bool &tailcall);
	bool TailCall(SQClosure *closure, int64_t firstparam, int64_t nparams);
	//starts a RABBIT call in the same "Execution loop"
	bool StartCall(SQClosure *closure, int64_t target, int64_t nargs, int64_t stackbase, bool tailcall);
	bool createClassInstance(SQClass *theclass, SQObjectPtr &inst, SQObjectPtr &constructor);
	//call a generic closure pure RABBIT or NATIVE
	bool Call(SQObjectPtr &closure, int64_t nparams, int64_t stackbase, SQObjectPtr &outres,SQBool raiseerror);
	SQRESULT Suspend();

	void CallDebugHook(int64_t type,int64_t forcedline=0);
	void CallErrorHandler(SQObjectPtr &e);
	bool get(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, uint64_t getflags, int64_t selfidx);
	int64_t FallBackget(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
	bool InvokeDefaultDelegate(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest);
	bool set(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val, int64_t selfidx);
	int64_t FallBackset(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val);
	bool NewSlot(const SQObjectPtr &self, const SQObjectPtr &key, const SQObjectPtr &val,bool bstatic);
	bool NewSlotA(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,const SQObjectPtr &attrs,bool bstatic,bool raw);
	bool DeleteSlot(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &res);
	bool clone(const SQObjectPtr &self, SQObjectPtr &target);
	bool ObjCmp(const SQObjectPtr &o1, const SQObjectPtr &o2,int64_t &res);
	bool StringCat(const SQObjectPtr &str, const SQObjectPtr &obj, SQObjectPtr &dest);
	static bool IsEqual(const SQObjectPtr &o1,const SQObjectPtr &o2,bool &res);
	bool ToString(const SQObjectPtr &o,SQObjectPtr &res);
	SQString *PrintObjVal(const SQObjectPtr &o);


	void Raise_Error(const SQChar *s, ...);
	void Raise_Error(const SQObjectPtr &desc);
	void Raise_IdxError(const SQObjectPtr &o);
	void Raise_CompareError(const SQObject &o1, const SQObject &o2);
	void Raise_ParamTypeError(int64_t nparam,int64_t typemask,int64_t type);

	void FindOuter(SQObjectPtr &target, SQObjectPtr *stackindex);
	void RelocateOuters();
	void CloseOuters(SQObjectPtr *stackindex);

	bool TypeOf(const SQObjectPtr &obj1, SQObjectPtr &dest);
	bool CallMetaMethod(SQObjectPtr &closure, SQMetaMethod mm, int64_t nparams, SQObjectPtr &outres);
	bool ArithMetaMethod(int64_t op, const SQObjectPtr &o1, const SQObjectPtr &o2, SQObjectPtr &dest);
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
	bool DerefInc(int64_t op,SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix,int64_t arg0);
#ifdef _DEBUG_DUMP
	void dumpstack(int64_t stackbase=-1, bool dumpall = false);
#endif

	void Finalize();
	void GrowCallStack() {
		int64_t newsize = _alloccallsstacksize*2;
		_callstackdata.resize(newsize);
		_callsstack = &_callstackdata[0];
		_alloccallsstacksize = newsize;
	}
	bool EnterFrame(int64_t newbase, int64_t newtop, bool tailcall);
	void LeaveFrame();
	void release(){ sq_delete(this,SQVM); }
////////////////////////////////////////////////////////////////////////////
	//stack functions for the api
	void remove(int64_t n);

	static bool IsFalse(SQObjectPtr &o);

	void Pop();
	void Pop(int64_t n);
	void Push(const SQObjectPtr &o);
	void PushNull();
	SQObjectPtr &top();
	SQObjectPtr &Popget();
	SQObjectPtr &getUp(int64_t n);
	SQObjectPtr &getAt(int64_t n);

	SQObjectPtrVec _stack;

	int64_t _top;
	int64_t _stackbase;
	SQOuter *_openouters;
	SQObjectPtr _roottable;
	SQObjectPtr _lasterror;
	SQObjectPtr _errorhandler;

	bool _debughook;
	SQDEBUGHOOK _debughook_native;
	SQObjectPtr _debughook_closure;

	SQObjectPtr temp_reg;


	CallInfo* _callsstack;
	int64_t _callsstacksize;
	int64_t _alloccallsstacksize;
	sqvector<CallInfo>  _callstackdata;

	ExceptionsTraps _etraps;
	CallInfo *ci;
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

struct AutoDec{
	AutoDec(int64_t *n) { _n = n; }
	~AutoDec() { (*_n)--; }
	int64_t *_n;
};

inline SQObjectPtr &stack_get(HRABBITVM v,int64_t idx){return ((idx>=0)?(v->getAt(idx+v->_stackbase-1)):(v->getUp(idx)));}

#define _ss(_vm_) (_vm_)->_sharedstate

#define PUSH_CALLINFO(v,nci){ \
	int64_t css = v->_callsstacksize; \
	if(css == v->_alloccallsstacksize) { \
		v->GrowCallStack(); \
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
