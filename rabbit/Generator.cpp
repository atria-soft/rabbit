/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Generator.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/squtils.hpp>






bool rabbit::Generator::yield(rabbit::VirtualMachine *v,int64_t target)
{
	if(_state==eSuspended) { v->raise_error("internal vm error, yielding dead generator");  return false;}
	if(_state==eDead) { v->raise_error("internal vm error, yielding a dead generator"); return false; }
	int64_t size = v->_top-v->_stackbase;

	_stack.resize(size);
	rabbit::Object _this = v->_stack[v->_stackbase];
	_stack[0] = ISREFCOUNTED(sq_type(_this)) ? rabbit::ObjectPtr(_refcounted(_this)->getWeakRef(sq_type(_this))) : _this;
	for(int64_t n =1; n<target; n++) {
		_stack[n] = v->_stack[v->_stackbase+n];
	}
	for(int64_t j =0; j < size; j++)
	{
		v->_stack[v->_stackbase+j].Null();
	}

	_ci = *v->ci;
	_ci._generator=NULL;
	for(int64_t i=0;i<_ci._etraps;i++) {
		_etraps.pushBack(v->_etraps.back());
		v->_etraps.popBack();
		// store relative stack base and size in case of resume to other _top
		rabbit::ExceptionTrap &et = _etraps.back();
		et._stackbase -= v->_stackbase;
		et._stacksize -= v->_stackbase;
	}
	_state=eSuspended;
	return true;
}

bool rabbit::Generator::resume(rabbit::VirtualMachine *v,rabbit::ObjectPtr &dest)
{
	if(_state==eDead){ v->raise_error("resuming dead generator"); return false; }
	if(_state==eRunning){ v->raise_error("resuming active generator"); return false; }
	int64_t size = _stack.size();
	int64_t target = &dest - &(v->_stack[v->_stackbase]);
	assert(target>=0 && target<=255);
	int64_t newbase = v->_top;
	if(!v->enterFrame(v->_top, v->_top + size, false))
		return false;
	v->ci->_generator   = this;
	v->ci->_target	  = (int32_t)target;
	v->ci->_closure	 = _ci._closure;
	v->ci->_ip		  = _ci._ip;
	v->ci->_literals	= _ci._literals;
	v->ci->_ncalls	  = _ci._ncalls;
	v->ci->_etraps	  = _ci._etraps;
	v->ci->_root		= _ci._root;


	for(int64_t i=0;i<_ci._etraps;i++) {
		v->_etraps.pushBack(_etraps.back());
		_etraps.popBack();
		rabbit::ExceptionTrap &et = v->_etraps.back();
		// restore absolute stack base and size
		et._stackbase += newbase;
		et._stacksize += newbase;
	}
	rabbit::Object _this = _stack[0];
	v->_stack[v->_stackbase] = sq_type(_this) == rabbit::OT_WEAKREF ? _weakref(_this)->_obj : _this;

	for(int64_t n = 1; n<size; n++) {
		v->_stack[v->_stackbase+n] = _stack[n];
		_stack[n].Null();
	}

	_state=eRunning;
	if (v->_debughook)
		v->callDebugHook('c');

	return true;
}

rabbit::Generator::Generator(rabbit::SharedState *ss,rabbit::Closure *closure) {
	_closure = closure;
	_state = eRunning;
	_ci._generator = NULL;
}

rabbit::Generator *rabbit::Generator::create(rabbit::SharedState *ss,rabbit::Closure *closure) {
	rabbit::Generator *nc=(rabbit::Generator*)SQ_MALLOC(sizeof(rabbit::Generator));
	new ((char*)nc) rabbit::Generator(ss,closure);
	return nc;
}

rabbit::Generator::~Generator() {
	
}

void rabbit::Generator::kill() {
	_state=eDead;
	_stack.resize(0);
	_closure.Null();
}

void rabbit::Generator::release() {
	sq_delete(this,Generator);
}

