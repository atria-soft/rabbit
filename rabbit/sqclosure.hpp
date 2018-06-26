/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#define _CALC_CLOSURE_SIZE(func) (sizeof(SQClosure) + (func->_noutervalues*sizeof(SQObjectPtr)) + (func->_ndefaultparams*sizeof(SQObjectPtr)))

struct SQFunctionProto;
struct SQClass;
struct SQClosure : public rabbit::RefCounted
{
private:
	SQClosure(SQSharedState *ss,SQFunctionProto *func){
		_function = func;
		__ObjAddRef(_function); _base = NULL;
		_env = NULL;
		_root=NULL;
	}
public:
	static SQClosure *create(SQSharedState *ss,SQFunctionProto *func,rabbit::WeakRef *root){
		int64_t size = _CALC_CLOSURE_SIZE(func);
		SQClosure *nc=(SQClosure*)SQ_MALLOC(size);
		new (nc) SQClosure(ss,func);
		nc->_outervalues = (SQObjectPtr *)(nc + 1);
		nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
		nc->_root = root;
		 __ObjAddRef(nc->_root);
		_CONSTRUCT_VECTOR(SQObjectPtr,func->_noutervalues,nc->_outervalues);
		_CONSTRUCT_VECTOR(SQObjectPtr,func->_ndefaultparams,nc->_defaultparams);
		return nc;
	}
	void release(){
		SQFunctionProto *f = _function;
		int64_t size = _CALC_CLOSURE_SIZE(f);
		_DESTRUCT_VECTOR(SQObjectPtr,f->_noutervalues,_outervalues);
		_DESTRUCT_VECTOR(SQObjectPtr,f->_ndefaultparams,_defaultparams);
		__Objrelease(_function);
		this->~SQClosure();
		sq_vm_free(this,size);
	}
	void setRoot(rabbit::WeakRef *r)
	{
		__Objrelease(_root);
		_root = r;
		__ObjAddRef(_root);
	}
	SQClosure *clone()
	{
		SQFunctionProto *f = _function;
		SQClosure * ret = SQClosure::create(NULL,f,_root);
		ret->_env = _env;
		if(ret->_env) __ObjAddRef(ret->_env);
		_COPY_VECTOR(ret->_outervalues,_outervalues,f->_noutervalues);
		_COPY_VECTOR(ret->_defaultparams,_defaultparams,f->_ndefaultparams);
		return ret;
	}
	~SQClosure();

	bool Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write);
	static bool Load(SQVM *v,SQUserPointer up,SQREADFUNC read,SQObjectPtr &ret);
	rabbit::WeakRef *_env;
	rabbit::WeakRef *_root;
	SQClass *_base;
	SQFunctionProto *_function;
	SQObjectPtr *_outervalues;
	SQObjectPtr *_defaultparams;
};

//////////////////////////////////////////////
struct SQOuter : public rabbit::RefCounted
{

private:
	SQOuter(SQSharedState *ss, SQObjectPtr *outer){
		_valptr = outer;
		_next = NULL;
	}

public:
	static SQOuter *create(SQSharedState *ss, SQObjectPtr *outer)
	{
		SQOuter *nc  = (SQOuter*)SQ_MALLOC(sizeof(SQOuter));
		new (nc) SQOuter(ss, outer);
		return nc;
	}
	~SQOuter() {
		
	}

	void release()
	{
		this->~SQOuter();
		sq_vm_free(this,sizeof(SQOuter));
	}

	SQObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
	int64_t	_idx;	 /* idx in stack array, for relocation */
	SQObjectPtr  _value;   /* value of outer after stack frame is closed */
	SQOuter	 *_next;	/* pointer to next outer when frame is open   */
};

//////////////////////////////////////////////
struct SQGenerator : public rabbit::RefCounted
{
	enum SQGeneratorState{eRunning,eSuspended,eDead};
private:
	SQGenerator(SQSharedState *ss,SQClosure *closure){
		_closure=closure;
		_state=eRunning;
		_ci._generator=NULL;
	}
public:
	static SQGenerator *create(SQSharedState *ss,SQClosure *closure){
		SQGenerator *nc=(SQGenerator*)SQ_MALLOC(sizeof(SQGenerator));
		new (nc) SQGenerator(ss,closure);
		return nc;
	}
	~SQGenerator()
	{
		
	}
	void Kill(){
		_state=eDead;
		_stack.resize(0);
		_closure.Null();}
	void release(){
		sq_delete(this,SQGenerator);
	}

	bool Yield(SQVM *v,int64_t target);
	bool Resume(SQVM *v,SQObjectPtr &dest);
	SQObjectPtr _closure;
	SQObjectPtrVec _stack;
	SQVM::CallInfo _ci;
	ExceptionsTraps _etraps;
	SQGeneratorState _state;
};

#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(SQNativeClosure) + (noutervalues*sizeof(SQObjectPtr)))

struct SQNativeClosure : public rabbit::RefCounted
{
private:
	SQNativeClosure(SQSharedState *ss,SQFUNCTION func){
		_function=func;
		_env = NULL;
	}
public:
	static SQNativeClosure *create(SQSharedState *ss,SQFUNCTION func,int64_t nouters)
	{
		int64_t size = _CALC_NATVIVECLOSURE_SIZE(nouters);
		SQNativeClosure *nc=(SQNativeClosure*)SQ_MALLOC(size);
		new (nc) SQNativeClosure(ss,func);
		nc->_outervalues = (SQObjectPtr *)(nc + 1);
		nc->_noutervalues = nouters;
		_CONSTRUCT_VECTOR(SQObjectPtr,nc->_noutervalues,nc->_outervalues);
		return nc;
	}
	SQNativeClosure *clone()
	{
		SQNativeClosure * ret = SQNativeClosure::create(NULL,_function,_noutervalues);
		ret->_env = _env;
		if(ret->_env) __ObjAddRef(ret->_env);
		ret->_name = _name;
		_COPY_VECTOR(ret->_outervalues,_outervalues,_noutervalues);
		ret->_typecheck.copy(_typecheck);
		ret->_nparamscheck = _nparamscheck;
		return ret;
	}
	~SQNativeClosure()
	{
		__Objrelease(_env);
	}
	void release(){
		int64_t size = _CALC_NATVIVECLOSURE_SIZE(_noutervalues);
		_DESTRUCT_VECTOR(SQObjectPtr,_noutervalues,_outervalues);
		this->~SQNativeClosure();
		sq_free(this,size);
	}

	int64_t _nparamscheck;
	SQIntVec _typecheck;
	SQObjectPtr *_outervalues;
	uint64_t _noutervalues;
	rabbit::WeakRef *_env;
	SQFUNCTION _function;
	SQObjectPtr _name;
};


