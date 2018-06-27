/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#define _CALC_CLOSURE_SIZE(func) (sizeof(SQClosure) + (func->_noutervalues*sizeof(rabbit::ObjectPtr)) + (func->_ndefaultparams*sizeof(rabbit::ObjectPtr)))

struct SQFunctionProto;
struct SQClass;
struct SQClosure : public rabbit::RefCounted
{
private:
	SQClosure(SQSharedState *ss,SQFunctionProto *func){
		_function = func;
		__ObjaddRef(_function); _base = NULL;
		_env = NULL;
		_root=NULL;
	}
public:
	static SQClosure *create(SQSharedState *ss,SQFunctionProto *func,rabbit::WeakRef *root){
		int64_t size = _CALC_CLOSURE_SIZE(func);
		SQClosure *nc=(SQClosure*)SQ_MALLOC(size);
		new (nc) SQClosure(ss,func);
		nc->_outervalues = (rabbit::ObjectPtr *)(nc + 1);
		nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
		nc->_root = root;
		 __ObjaddRef(nc->_root);
		_CONSTRUCT_VECTOR(rabbit::ObjectPtr,func->_noutervalues,nc->_outervalues);
		_CONSTRUCT_VECTOR(rabbit::ObjectPtr,func->_ndefaultparams,nc->_defaultparams);
		return nc;
	}
	void release(){
		SQFunctionProto *f = _function;
		int64_t size = _CALC_CLOSURE_SIZE(f);
		_DESTRUCT_VECTOR(rabbit::ObjectPtr,f->_noutervalues,_outervalues);
		_DESTRUCT_VECTOR(rabbit::ObjectPtr,f->_ndefaultparams,_defaultparams);
		__Objrelease(_function);
		this->~SQClosure();
		sq_vm_free(this,size);
	}
	void setRoot(rabbit::WeakRef *r)
	{
		__Objrelease(_root);
		_root = r;
		__ObjaddRef(_root);
	}
	SQClosure *clone()
	{
		SQFunctionProto *f = _function;
		SQClosure * ret = SQClosure::create(NULL,f,_root);
		ret->_env = _env;
		if(ret->_env) __ObjaddRef(ret->_env);
		_COPY_VECTOR(ret->_outervalues,_outervalues,f->_noutervalues);
		_COPY_VECTOR(ret->_defaultparams,_defaultparams,f->_ndefaultparams);
		return ret;
	}
	~SQClosure();

	bool save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write);
	static bool load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret);
	rabbit::WeakRef *_env;
	rabbit::WeakRef *_root;
	SQClass *_base;
	SQFunctionProto *_function;
	rabbit::ObjectPtr *_outervalues;
	rabbit::ObjectPtr *_defaultparams;
};

//////////////////////////////////////////////
struct SQOuter : public rabbit::RefCounted
{

private:
	SQOuter(SQSharedState *ss, rabbit::ObjectPtr *outer){
		_valptr = outer;
		_next = NULL;
	}

public:
	static SQOuter *create(SQSharedState *ss, rabbit::ObjectPtr *outer)
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

	rabbit::ObjectPtr *_valptr;  /* pointer to value on stack, or _value below */
	int64_t	_idx;	 /* idx in stack array, for relocation */
	rabbit::ObjectPtr  _value;   /* value of outer after stack frame is closed */
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
	void kill(){
		_state=eDead;
		_stack.resize(0);
		_closure.Null();}
	void release(){
		sq_delete(this,SQGenerator);
	}

	bool yield(rabbit::VirtualMachine *v,int64_t target);
	bool resume(rabbit::VirtualMachine *v,rabbit::ObjectPtr &dest);
	rabbit::ObjectPtr _closure;
	etk::Vector<rabbit::ObjectPtr> _stack;
	rabbit::VirtualMachine::callInfo _ci;
	etk::Vector<rabbit::ExceptionTrap> _etraps;
	SQGeneratorState _state;
};

#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(SQNativeClosure) + (noutervalues*sizeof(rabbit::ObjectPtr)))

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
		nc->_outervalues = (rabbit::ObjectPtr *)(nc + 1);
		nc->_noutervalues = nouters;
		_CONSTRUCT_VECTOR(rabbit::ObjectPtr,nc->_noutervalues,nc->_outervalues);
		return nc;
	}
	SQNativeClosure *clone()
	{
		SQNativeClosure * ret = SQNativeClosure::create(NULL,_function,_noutervalues);
		ret->_env = _env;
		if(ret->_env) __ObjaddRef(ret->_env);
		ret->_name = _name;
		_COPY_VECTOR(ret->_outervalues,_outervalues,_noutervalues);
		ret->_typecheck = _typecheck;
		ret->_nparamscheck = _nparamscheck;
		return ret;
	}
	~SQNativeClosure()
	{
		__Objrelease(_env);
	}
	void release(){
		int64_t size = _CALC_NATVIVECLOSURE_SIZE(_noutervalues);
		_DESTRUCT_VECTOR(rabbit::ObjectPtr,_noutervalues,_outervalues);
		this->~SQNativeClosure();
		sq_free(this,size);
	}

	int64_t _nparamscheck;
	etk::Vector<int64_t> _typecheck;
	rabbit::ObjectPtr *_outervalues;
	uint64_t _noutervalues;
	rabbit::WeakRef *_env;
	SQFUNCTION _function;
	rabbit::ObjectPtr _name;
};


