/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <math.h>
#include <stdlib.h>
#include <rabbit/sqopcodes.hpp>
#include <rabbit/VirtualMachine.hpp>


#include <rabbit/squtils.hpp>


#include <rabbit/UserData.hpp>
#include <rabbit/Array.hpp>
#include <rabbit/Instance.hpp>
#include <rabbit/Closure.hpp>
#include <rabbit/String.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/Generator.hpp>
#include <rabbit/Class.hpp>
#include <rabbit/FunctionProto.hpp>
#include <rabbit/NativeClosure.hpp>
#include <rabbit/WeakRef.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/Outer.hpp>


#define TOP() (_stack[_top-1])
#define TARGET _stack[_stackbase+arg0]
#define STK(a) _stack[_stackbase+(a)]

bool rabbit::VirtualMachine::BW_OP(uint64_t op,rabbit::ObjectPtr &trg,const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2)
{
	int64_t res;
	if((o1.getType()| o2.getType()) == rabbit::OT_INTEGER)
	{
		int64_t i1 = o1.toInteger(), i2 = o2.toInteger();
		switch(op) {
			case BW_AND:
				res = i1 & i2;
				break;
			case BW_OR:
				res = i1 | i2;
				break;
			case BW_XOR:
				res = i1 ^ i2;
				break;
			case BW_SHIFTL:
				res = i1 << i2;
				break;
			case BW_SHIFTR:
				res = i1 >> i2;
				break;
			case BW_USHIFTR:
				res = (int64_t)(*((uint64_t*)&i1) >> i2);
				break;
			default: { raise_error("internal vm error bitwise op failed"); return false; }
		}
	}
	else { raise_error("bitwise op between '%s' and '%s'",getTypeName(o1),getTypeName(o2)); return false;}
	trg = res;
	return true;
}

void rabbit::VirtualMachine::release() {
	ETK_DELETE(VirtualMachine, this);
}

#define _ARITH_(op,trg,o1,o2) \
{ \
	int64_t tmask = o1.getType()|o2.getType(); \
	switch(tmask) { \
		case rabbit::OT_INTEGER: trg = o1.toInteger() op o2.toInteger();break; \
		case (rabbit::OT_FLOAT|OT_INTEGER): \
		case (rabbit::OT_FLOAT): trg = o1.toFloatValue() op o2.toFloatValue(); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

#define _ARITH_NOZERO(op,trg,o1,o2,err) \
{ \
	int64_t tmask = o1.getType()|o2.getType(); \
	switch(tmask) { \
		case rabbit::OT_INTEGER: { int64_t i2 = o2.toInteger(); if(i2 == 0) { raise_error(err); SQ_THROW(); } trg = o1.toInteger() op i2; } break;\
		case (rabbit::OT_FLOAT|OT_INTEGER): \
		case (rabbit::OT_FLOAT): trg = o1.toFloatValue() op o2.toFloatValue(); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

bool rabbit::VirtualMachine::ARITH_OP(uint64_t op,rabbit::ObjectPtr &trg,const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2)
{
	int64_t tmask = o1.getType() | o2.getType();
	switch(tmask) {
		case rabbit::OT_INTEGER:{
			int64_t res, i1 = o1.toInteger(), i2 = o2.toInteger();
			switch(op) {
			case '+': res = i1 + i2; break;
			case '-': res = i1 - i2; break;
			case '/': if (i2 == 0) { raise_error("division by zero"); return false; }
					else if (i2 == -1 && i1 == INT64_MIN) { raise_error("integer overflow"); return false; }
					res = i1 / i2;
					break;
			case '*': res = i1 * i2; break;
			case '%': if (i2 == 0) { raise_error("modulo by zero"); return false; }
					else if (i2 == -1 && i1 == INT64_MAX) { res = 0; break; }
					res = i1 % i2;
					break;
			default: res = 0xDEADBEEF;
			}
			trg = res; }
			break;
		case (rabbit::OT_FLOAT|OT_INTEGER):
		case (rabbit::OT_FLOAT):{
			float_t res, f1 = o1.toFloatValue(), f2 = o2.toFloatValue();
			switch(op) {
				case '+': res = f1 + f2; break;
				case '-': res = f1 - f2; break;
				case '/': res = f1 / f2; break;
				case '*': res = f1 * f2; break;
				case '%': res = float_t(fmod((double)f1,(double)f2)); break;
				default: res = 0x0f;
			}
			trg = res; }
			break;
		default:
			if(op == '+' && (tmask & _RT_STRING)){
				if(!stringCat(o1, o2, trg)) return false;
			}
			else if(!arithMetaMethod(op,o1,o2,trg)) {
				return false;
			}
	}
	return true;
}

rabbit::VirtualMachine::VirtualMachine(rabbit::SharedState *ss)
{
	_sharedstate=ss;
	_suspended = SQFalse;
	_suspended_target = -1;
	_suspended_root = SQFalse;
	_suspended_traps = -1;
	_foreignptr = NULL;
	_nnativecalls = 0;
	_nmetamethodscall = 0;
	_lasterror.Null();
	_errorhandler.Null();
	_debughook = false;
	_debughook_native = NULL;
	_debughook_closure.Null();
	_openouters = NULL;
	ci = NULL;
	_releasehook = NULL;
}

void rabbit::VirtualMachine::finalize()
{
	if(_releasehook) { _releasehook(_foreignptr,0); _releasehook = NULL; }
	if(_openouters) closeOuters(&_stack[0]);
	_roottable.Null();
	_lasterror.Null();
	_errorhandler.Null();
	_debughook = false;
	_debughook_native = NULL;
	_debughook_closure.Null();
	temp_reg.Null();
	_callstackdata.resize(0);
	int64_t size=_stack.size();
	for(int64_t i=0;i<size;i++)
		_stack[i].Null();
}

rabbit::VirtualMachine::~VirtualMachine()
{
	finalize();
}

bool rabbit::VirtualMachine::arithMetaMethod(int64_t op,const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2,rabbit::ObjectPtr &dest)
{
	rabbit::MetaMethod mm;
	switch(op){
		case '+': mm=MT_ADD; break;
		case '-': mm=MT_SUB; break;
		case '/': mm=MT_DIV; break;
		case '*': mm=MT_MUL; break;
		case '%': mm=MT_MODULO; break;
		default: mm = MT_ADD; assert(0); break; //shutup compiler
	}
	if(    o1.isDelegable() == true
	    && o1.toDelegable()->_delegate) {
		rabbit::ObjectPtr closure;
		if(o1.toDelegable()->getMetaMethod(this, mm, closure)) {
			push(o1);
			push(o2);
			return callMetaMethod(closure,mm,2,dest);
		}
	}
	raise_error("arith op %c on between '%s' and '%s'",op,getTypeName(o1),getTypeName(o2));
	return false;
}

bool rabbit::VirtualMachine::NEG_OP(rabbit::ObjectPtr &trg,const rabbit::ObjectPtr &o)
{

	switch(o.getType()) {
		case rabbit::OT_INTEGER:
			trg = -o.toInteger();
			return true;
		case rabbit::OT_FLOAT:
			trg = -o.toFloat();
			return true;
		case rabbit::OT_TABLE:
		case rabbit::OT_USERDATA:
		case rabbit::OT_INSTANCE:
			if(o.toDelegable()->_delegate) {
				rabbit::ObjectPtr closure;
				if(o.toDelegable()->getMetaMethod(this, MT_UNM, closure)) {
					push(o);
					if(callMetaMethod(closure, MT_UNM, 1, temp_reg) == false) {
						return false;
					}
					trg.swap(temp_reg);
					return true;
				}
			}
		default:
			break; //shutup compiler
	}
	raise_error("attempt to negate a %s", getTypeName(o));
	return false;
}

#define _RET_SUCCEED(exp) { result = (exp); return true; }
bool rabbit::VirtualMachine::objCmp(const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2,int64_t &result)
{
	rabbit::ObjectType t1 = o1.getType();
	rabbit::ObjectType t2 = o2.getType();
	if(t1 == t2) {
		if(o1.toRaw() == o2.toRaw())_RET_SUCCEED(0);
		rabbit::ObjectPtr res;
		switch(t1){
		case rabbit::OT_STRING:
			_RET_SUCCEED(strcmp(o1.getStringValue(),o2.getStringValue()));
		case rabbit::OT_INTEGER:
			_RET_SUCCEED((o1.toInteger()<o2.toInteger())?-1:1);
		case rabbit::OT_FLOAT:
			_RET_SUCCEED((o1.toFloat()<o2.toFloat())?-1:1);
		case rabbit::OT_TABLE:
		case rabbit::OT_USERDATA:
		case rabbit::OT_INSTANCE:
			if(o1.toDelegable()->_delegate) {
				rabbit::ObjectPtr closure;
				if(o1.toDelegable()->getMetaMethod(this, MT_CMP, closure)) {
					push(o1);push(o2);
					if(callMetaMethod(closure,MT_CMP,2,res)) {
						if(res.isInteger() == false) {
							raise_error("_cmp must return an integer");
							return false;
						}
						_RET_SUCCEED(res.toInteger())
					}
					return false;
				}
			}
			//continues through (no break needed)
		default:
			_RET_SUCCEED( o1.toUserPointer() < o2.toUserPointer()?-1:1 );
		}
		assert(0);
		//if(type(res)!=rabbit::OT_INTEGER) { raise_Compareerror(o1,o2); return false; }
		//  _RET_SUCCEED(res.toInteger());

	}
	else{
		if(    o1.isNumeric() ==true
		    && o2.isNumeric() == true){
			if((t1==rabbit::OT_INTEGER) && (t2==OT_FLOAT)) {
				if( o1.toInteger()==o2.toFloat() ) { _RET_SUCCEED(0); }
				else if( o1.toInteger()<o2.toFloat() ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
			else{
				if( o1.toFloat()==o2.toInteger() ) { _RET_SUCCEED(0); }
				else if( o1.toFloat()<o2.toInteger() ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
		}
		else if(t1==rabbit::OT_NULL) {_RET_SUCCEED(-1);}
		else if(t2==rabbit::OT_NULL) {_RET_SUCCEED(1);}
		else { raise_Compareerror(o1,o2); return false; }

	}
	assert(0);
	_RET_SUCCEED(0); //cannot happen
}

bool rabbit::VirtualMachine::CMP_OP(CmpOP op, const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2,rabbit::ObjectPtr &res)
{
	int64_t r;
	if(objCmp(o1,o2,r)) {
		switch(op) {
			case CMP_G: res = (r > 0); return true;
			case CMP_GE: res = (r >= 0); return true;
			case CMP_L: res = (r < 0); return true;
			case CMP_LE: res = (r <= 0); return true;
			case CMP_3W: res = r; return true;
		}
		assert(0);
	}
	return false;
}

bool rabbit::VirtualMachine::toString(const rabbit::ObjectPtr &o,rabbit::ObjectPtr &res)
{
	switch(o.getType()) {
		case rabbit::OT_STRING:
			res = o;
			return true;
		case rabbit::OT_FLOAT:
			snprintf(_sp(sq_rsl(NUMBER_UINT8_MAX+1)),sq_rsl(NUMBER_UINT8_MAX),"%g",o.toFloat());
			break;
		case rabbit::OT_INTEGER:
			snprintf(_sp(sq_rsl(NUMBER_UINT8_MAX+1)),sq_rsl(NUMBER_UINT8_MAX),_PRINT_INT_FMT,o.toInteger());
			break;
		case rabbit::OT_BOOL:
			snprintf(_sp(sq_rsl(6)),sq_rsl(6),o.toInteger()?"true":"false");
			break;
		case rabbit::OT_TABLE:
		case rabbit::OT_USERDATA:
		case rabbit::OT_INSTANCE:
			if(o.toDelegable()->_delegate) {
				rabbit::ObjectPtr closure;
				if(o.toDelegable()->getMetaMethod(this, MT_TOSTRING, closure)) {
					push(o);
					if(callMetaMethod(closure,MT_TOSTRING,1,res)) {
						if(res.isString() == true) {
							return true;
						}
					} else {
						return false;
					}
				}
			}
		default:
			snprintf(_sp(sq_rsl((sizeof(void*)*2)+NUMBER_UINT8_MAX)),sq_rsl((sizeof(void*)*2)+NUMBER_UINT8_MAX),"(%s : 0x%p)",getTypeName(o),(void*)o.toRaw());
	}
	res = rabbit::String::create(_get_shared_state(this),_spval);
	return true;
}


bool rabbit::VirtualMachine::stringCat(const rabbit::ObjectPtr &str,const rabbit::ObjectPtr &obj,rabbit::ObjectPtr &dest)
{
	rabbit::ObjectPtr a, b;
	if(!toString(str, a)) return false;
	if(!toString(obj, b)) return false;
	int64_t l = a.toString()->_len , ol = b.toString()->_len;
	char *s = _sp(sq_rsl(l + ol + 1));
	memcpy(s, a.getStringValue(), sq_rsl(l));
	memcpy(s + l, b.getStringValue(), sq_rsl(ol));
	dest = rabbit::String::create(_get_shared_state(this), _spval, l + ol);
	return true;
}

bool rabbit::VirtualMachine::typeOf(const rabbit::ObjectPtr &obj1,rabbit::ObjectPtr &dest)
{
	if(    obj1.isDelegable() == true
	    && obj1.toDelegable()->_delegate) {
		rabbit::ObjectPtr closure;
		if(obj1.toDelegable()->getMetaMethod(this, MT_TYPEOF, closure)) {
			push(obj1);
			return callMetaMethod(closure,MT_TYPEOF,1,dest);
		}
	}
	dest = rabbit::String::create(_get_shared_state(this),getTypeName(obj1));
	return true;
}

bool rabbit::VirtualMachine::init(rabbit::VirtualMachine *friendvm, int64_t stacksize)
{
	_stack.resize(stacksize);
	_alloccallsstacksize = 4;
	_callstackdata.resize(_alloccallsstacksize);
	_callsstacksize = 0;
	_callsstack = &_callstackdata[0];
	_stackbase = 0;
	_top = 0;
	if(!friendvm) {
		_roottable = rabbit::Table::create(_get_shared_state(this), 0);
		sq_base_register(this);
	}
	else {
		_roottable = friendvm->_roottable;
		_errorhandler = friendvm->_errorhandler;
		_debughook = friendvm->_debughook;
		_debughook_native = friendvm->_debughook_native;
		_debughook_closure = friendvm->_debughook_closure;
	}


	return true;
}


bool rabbit::VirtualMachine::startcall(rabbit::Closure *closure,int64_t target,int64_t args,int64_t stackbase,bool tailcall)
{
	rabbit::FunctionProto *func = closure->_function;

	int64_t paramssize = func->_nparameters;
	const int64_t newtop = stackbase + func->_stacksize;
	int64_t nargs = args;
	if(func->_varparams)
	{
		paramssize--;
		if (nargs < paramssize) {
			raise_error("wrong number of parameters");
			return false;
		}

		//dumpstack(stackbase);
		int64_t nvargs = nargs - paramssize;
		rabbit::Array *arr = rabbit::Array::create(_get_shared_state(this),nvargs);
		int64_t pbase = stackbase+paramssize;
		for(int64_t n = 0; n < nvargs; n++) {
			(*arr)[n] = _stack[pbase];
			_stack[pbase].Null();
			pbase++;

		}
		_stack[stackbase+paramssize] = arr;
		//dumpstack(stackbase);
	}
	else if (paramssize != nargs) {
		int64_t ndef = func->_ndefaultparams;
		int64_t diff;
		if(ndef && nargs < paramssize && (diff = paramssize - nargs) <= ndef) {
			for(int64_t n = ndef - diff; n < ndef; n++) {
				_stack[stackbase + (nargs++)] = closure->_defaultparams[n];
			}
		}
		else {
			raise_error("wrong number of parameters");
			return false;
		}
	}

	if(closure->_env) {
		_stack[stackbase] = closure->_env->_obj;
	}

	if(!enterFrame(stackbase, newtop, tailcall)) return false;

	ci->_closure  = closure;
	ci->_literals = func->_literals;
	ci->_ip	   = func->_instructions;
	ci->_target   = (int32_t)target;

	if (_debughook) {
		callDebugHook('c');
	}

	if (closure->_function->_bgenerator) {
		rabbit::FunctionProto *f = closure->_function;
		rabbit::Generator *gen = rabbit::Generator::create(_get_shared_state(this), closure);
		if(!gen->yield(this,f->_stacksize))
			return false;
		rabbit::ObjectPtr temp;
		Return(1, target, temp);
		STK(target) = gen;
	}


	return true;
}

bool rabbit::VirtualMachine::Return(int64_t _arg0, int64_t _arg1, rabbit::ObjectPtr &retval)
{
	rabbit::Bool	_isroot	  = ci->_root;
	int64_t callerbase   = _stackbase - ci->_prevstkbase;

	if (_debughook) {
		for(int64_t i=0; i<ci->_ncalls; i++) {
			callDebugHook('r');
		}
	}

	rabbit::ObjectPtr *dest;
	if (_isroot) {
		dest = &(retval);
	} else if (ci->_target == -1) {
		dest = NULL;
	} else {
		dest = &_stack[callerbase + ci->_target];
	}
	if (dest) {
		if(_arg0 != 0xFF) {
			*dest = _stack[_stackbase+_arg1];
		}
		else {
			dest->Null();
		}
		//*dest = (_arg0 != 0xFF) ? _stack[_stackbase+_arg1] : _null_;
	}
	leaveFrame();
	return _isroot ? true : false;
}

#define _RET_ON_FAIL(exp) { if(!exp) return false; }

bool rabbit::VirtualMachine::PLOCAL_INC(int64_t op,rabbit::ObjectPtr &target, rabbit::ObjectPtr &a, rabbit::ObjectPtr &incr)
{
	rabbit::ObjectPtr trg;
	_RET_ON_FAIL(ARITH_OP( op , trg, a, incr));
	target = a;
	a = trg;
	return true;
}

bool rabbit::VirtualMachine::derefInc(int64_t op,rabbit::ObjectPtr &target, rabbit::ObjectPtr &self, rabbit::ObjectPtr &key, rabbit::ObjectPtr &incr, bool postfix,int64_t selfidx)
{
	rabbit::ObjectPtr tmp, tself = self, tkey = key;
	if (!get(tself, tkey, tmp, 0, selfidx)) { return false; }
	_RET_ON_FAIL(ARITH_OP( op , target, tmp, incr))
	if (!set(tself, tkey, target,selfidx)) { return false; }
	if (postfix) target = tmp;
	return true;
}

#define arg0 (_i_._arg0)
#define sarg0 ((int64_t)*((const signed char *)&_i_._arg0))
#define arg1 (_i_._arg1)
#define sarg1 (*((const int32_t *)&_i_._arg1))
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)
#define sarg3 ((int64_t)*((const signed char *)&_i_._arg3))

rabbit::Result rabbit::VirtualMachine::Suspend()
{
	if (_suspended)
		return sq_throwerror(this, "cannot suspend an already suspended vm");
	if (_nnativecalls!=2)
		return sq_throwerror(this, "cannot suspend through native calls/metamethods");
	return SQ_SUSPEND_FLAG;
}


#define _FINISH(howmuchtojump) \
	{ \
		jump = howmuchtojump; \
		return true; \
	}

bool rabbit::VirtualMachine::FOREACH_OP(rabbit::ObjectPtr &o1,rabbit::ObjectPtr &o2,rabbit::ObjectPtr
&o3,rabbit::ObjectPtr &o4,int64_t SQ_UNUSED_ARG(arg_2),int exitpos,int &jump)
{
	int64_t nrefidx;
	switch(o1.getType()) {
		case rabbit::OT_TABLE:
			if((nrefidx = o1.toTable()->next(false,o4, o2, o3)) == -1) _FINISH(exitpos);
			o4 = (int64_t)nrefidx; _FINISH(1);
		case rabbit::OT_ARRAY:
			if((nrefidx = o1.toArray()->next(o4, o2, o3)) == -1) _FINISH(exitpos);
			o4 = (int64_t) nrefidx; _FINISH(1);
		case rabbit::OT_STRING:
			if((nrefidx = o1.toString()->next(o4, o2, o3)) == -1)_FINISH(exitpos);
			o4 = (int64_t)nrefidx; _FINISH(1);
		case rabbit::OT_CLASS:
			if((nrefidx = o1.toClass()->next(o4, o2, o3)) == -1)_FINISH(exitpos);
			o4 = (int64_t)nrefidx; _FINISH(1);
		case rabbit::OT_USERDATA:
		case rabbit::OT_INSTANCE:
			if(o1.toDelegable()->_delegate) {
				rabbit::ObjectPtr itr;
				rabbit::ObjectPtr closure;
				if(o1.toDelegable()->getMetaMethod(this, MT_NEXTI, closure)) {
					push(o1);
					push(o4);
					if(callMetaMethod(closure, MT_NEXTI, 2, itr)) {
						o4 = o2 = itr;
						if(itr.isNull() == true) {
							_FINISH(exitpos);
						}
						if(!get(o1, itr, o3, 0, DONT_FALL_BACK)) {
							raise_error("_nexti returned an invalid idx"); // cloud be changed
							return false;
						}
						_FINISH(1);
					}
					else {
						return false;
					}
				}
				raise_error("_nexti failed");
				return false;
			}
			break;
		case rabbit::OT_GENERATOR:
			if(o1.toGenerator()->_state == rabbit::Generator::eDead) {
				_FINISH(exitpos);
			}
			if(o1.toGenerator()->_state == rabbit::Generator::eSuspended) {
				int64_t idx = 0;
				if(o4.isInteger() == true) {
					idx = o4.toInteger() + 1;
				}
				o2 = idx;
				o4 = idx;
				o1.toGenerator()->resume(this, o3);
				_FINISH(0);
			}
		default:
			raise_error("cannot iterate %s", getTypeName(o1));
	}
	return false; //cannot be hit(just to avoid warnings)
}

#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))

#define SQ_THROW() { goto exception_trap; }

#define _GUARD(exp) { if(!exp) { SQ_THROW();} }

bool rabbit::VirtualMachine::CLOSURE_OP(rabbit::ObjectPtr &target, rabbit::FunctionProto *func)
{
	int64_t nouters;
	rabbit::Closure *closure = rabbit::Closure::create(_get_shared_state(this), func,_roottable.toTable()->getWeakRef(rabbit::OT_TABLE));
	if((nouters = func->_noutervalues)) {
		for(int64_t i = 0; i<nouters; i++) {
			rabbit::OuterVar &v = func->_outervalues[i];
			switch(v._type){
			case otLOCAL:
				findOuter(closure->_outervalues[i], &STK(v._src.toInteger()));
				break;
			case otOUTER:
				closure->_outervalues[i] = ci->_closure.toClosure()->_outervalues[v._src.toInteger()];
				break;
			}
		}
	}
	int64_t ndefparams;
	if((ndefparams = func->_ndefaultparams)) {
		for(int64_t i = 0; i < ndefparams; i++) {
			int64_t spos = func->_defaultparams[i];
			closure->_defaultparams[i] = _stack[_stackbase + spos];
		}
	}
	target = closure;
	return true;

}


bool rabbit::VirtualMachine::CLASS_OP(rabbit::ObjectPtr &target,int64_t baseclass,int64_t attributes)
{
	rabbit::Class *base = NULL;
	rabbit::ObjectPtr attrs;
	if(baseclass != -1) {
		if(_stack[_stackbase+baseclass].isClass() == false) {
			raise_error("trying to inherit from a %s",getTypeName(_stack[_stackbase+baseclass]));
			return false;
		}
		base = _stack[_stackbase + baseclass].toClass();
	}
	if(attributes != MAX_FUNC_STACKSIZE) {
		attrs = _stack[_stackbase+attributes];
	}
	target = rabbit::Class::create(_get_shared_state(this),base);
	if(target.toClass()->_metamethods[MT_INHERITED].isNull() == false) {
		int nparams = 2;
		rabbit::ObjectPtr ret;
		push(target); push(attrs);
		if(!call(target.toClass()->_metamethods[MT_INHERITED],nparams,_top - nparams, ret, false)) {
			pop(nparams);
			return false;
		}
		pop(nparams);
	}
	target.toClass()->_attributes = attrs;
	return true;
}

bool rabbit::VirtualMachine::isEqual(const rabbit::ObjectPtr &o1,const rabbit::ObjectPtr &o2,bool &res)
{
	if(o1.getType() == o2.getType()) {
		res = (o1.toRaw() == o2.toRaw());
	} else {
		if(    o1.isNumeric() == true
		    && o2.isNumeric() == true) {
			res = o1.toFloatValue() == o2.toFloatValue();
		} else {
			res = false;
		}
	}
	return true;
}

bool rabbit::VirtualMachine::IsFalse(rabbit::ObjectPtr &o)
{
	// this is really strange ...
	if (    (    o.canBeFalse() == true
	          && (    o.isFloat() == true
	               && o.toFloat() == 0.0f
	             )
	        )
	     || o.toInteger() == 0
	   )
	{
		return true;
	}
	return false;
}

extern rabbit::InstructionDesc g_InstrDesc[];
bool rabbit::VirtualMachine::execute(rabbit::ObjectPtr &closure, int64_t nargs, int64_t stackbase,rabbit::ObjectPtr &outres, rabbit::Bool raiseerror,ExecutionType et)
{
	if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) { raise_error("Native stack overflow"); return false; }
	_nnativecalls++;
	AutoDec ad(&_nnativecalls);
	int64_t traps = 0;
	callInfo *prevci = ci;

	switch(et) {
		case ET_CALL: {
			temp_reg = closure;
			if(!startcall(temp_reg.toClosure(), _top - nargs, nargs, stackbase, false)) {
				//call the handler if there are no calls in the stack, if not relies on the previous node
				if(ci == NULL) callerrorHandler(_lasterror);
				return false;
			}
			if(ci == prevci) {
				outres = STK(_top-nargs);
				return true;
			}
			ci->_root = SQTrue;
					  }
			break;
		case ET_RESUME_GENERATOR: closure.toGenerator()->resume(this, outres); ci->_root = SQTrue; traps += ci->_etraps; break;
		case ET_RESUME_VM:
		case ET_RESUME_THROW_VM:
			traps = _suspended_traps;
			ci->_root = _suspended_root;
			_suspended = SQFalse;
			if(et  == ET_RESUME_THROW_VM) { SQ_THROW(); }
			break;
	}

exception_restore:
	//
	{
		for(;;)
		{
			const rabbit::Instruction &_i_ = *ci->_ip++;
			//dumpstack(_stackbase);
			//printf("\n[%d] %s %d %d %d %d\n",ci->_ip-ci->_closure.toClosure()->_function->_instructions,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
			switch(_i_.op)
			{
			case _OP_LINE:
				if (_debughook) {
					callDebugHook('l',arg1);
				}
				continue;
			case _OP_LOAD:
				TARGET = ci->_literals[arg1];
				continue;
			case _OP_LOADINT:
				TARGET = (int64_t)((int32_t)arg1);
				continue;
			case _OP_LOADFLOAT: TARGET = *((const float_t *)&arg1); continue;
			case _OP_DLOAD: TARGET = ci->_literals[arg1]; STK(arg2) = ci->_literals[arg3];continue;
			case _OP_TAILCALL:{
				rabbit::ObjectPtr &t = STK(arg1);
				if (    t.isClosure() == true
				     && !t.toClosure()->_function->_bgenerator ){
					rabbit::ObjectPtr clo = t;
					int64_t last_top = _top;
					if(_openouters) closeOuters(&(_stack[_stackbase]));
					for (int64_t i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
					_GUARD(startcall(clo.toClosure(), ci->_target, arg3, _stackbase, true));
					if (last_top >= _top) {
						_top = last_top;
					}
					continue;
				}
							  }
			case _OP_CALL: {
					rabbit::ObjectPtr clo = STK(arg1);
					switch (clo.getType()) {
						case rabbit::OT_CLOSURE:
							_GUARD(startcall(clo.toClosure(), sarg0, arg3, _stackbase+arg2, false));
							continue;
						case rabbit::OT_NATIVECLOSURE:
							{
								bool suspend;
								bool tailcall;
								_GUARD(callNative(clo.toNativeClosure(), arg3, _stackbase+arg2, clo, (int32_t)sarg0, suspend, tailcall));
								if(suspend){
									_suspended = SQTrue;
									_suspended_target = sarg0;
									_suspended_root = ci->_root;
									_suspended_traps = traps;
									outres = clo;
									return true;
								}
								if(sarg0 != -1 && !tailcall) {
									STK(arg0) = clo;
								}
							}
							continue;
						case rabbit::OT_CLASS:
							{
								rabbit::ObjectPtr inst;
								_GUARD(createClassInstance(clo.toClass(),inst,clo));
								if(sarg0 != -1) {
									STK(arg0) = inst;
								}
								int64_t stkbase;
								switch(clo.getType()) {
									case rabbit::OT_CLOSURE:
										stkbase = _stackbase+arg2;
										_stack[stkbase] = inst;
										_GUARD(startcall(clo.toClosure(), -1, arg3, stkbase, false));
										break;
									case rabbit::OT_NATIVECLOSURE:
										bool dummy;
										stkbase = _stackbase+arg2;
										_stack[stkbase] = inst;
										_GUARD(callNative(clo.toNativeClosure(), arg3, stkbase, clo, -1, dummy, dummy));
										break;
									default:
										break; //shutup GCC 4.x
								}
							}
							break;
						case rabbit::OT_TABLE:
						case rabbit::OT_USERDATA:
						case rabbit::OT_INSTANCE:
							{
								rabbit::ObjectPtr closure;
								if(clo.toDelegable()->_delegate && clo.toDelegable()->getMetaMethod(this,MT_CALL,closure)) {
									push(clo);
									for (int64_t i = 0; i < arg3; i++) push(STK(arg2 + i));
									if(!callMetaMethod(closure, MT_CALL, arg3+1, clo)) SQ_THROW();
									if(sarg0 != -1) {
										STK(arg0) = clo;
									}
									break;
								}
								//raise_error("attempt to call '%s'", getTypeName(clo));
								//SQ_THROW();
							}
						default:
							raise_error("attempt to call '%s'", getTypeName(clo));
							SQ_THROW();
					}
				}
				  continue;
			case _OP_PREPCALL:
			case _OP_PREPCALLK: {
					rabbit::ObjectPtr &key = _i_.op == _OP_PREPCALLK?(ci->_literals)[arg1]:STK(arg1);
					rabbit::ObjectPtr &o = STK(arg2);
					if (!get(o, key, temp_reg,0,arg2)) {
						SQ_THROW();
					}
					STK(arg3) = o;
					TARGET.swap(temp_reg);
				}
				continue;
			case _OP_GETK:
				if (!get(STK(arg2), ci->_literals[arg1], temp_reg, 0,arg2)) {
					SQ_THROW();
				}
				TARGET.swap(temp_reg);
				continue;
			case _OP_MOVE: TARGET = STK(arg1); continue;
			case _OP_NEWSLOT:
				_GUARD(newSlot(STK(arg1), STK(arg2), STK(arg3),false));
				if(arg0 != 0xFF) {
					TARGET = STK(arg3);
				}
				continue;
			case _OP_DELETE: _GUARD(deleteSlot(STK(arg1), STK(arg2), TARGET)); continue;
			case _OP_SET:
				if (!set(STK(arg1), STK(arg2), STK(arg3),arg1)) { SQ_THROW(); }
				if (arg0 != 0xFF) TARGET = STK(arg3);
				continue;
			case _OP_GET:
				if (!get(STK(arg1), STK(arg2), temp_reg, 0,arg1)) { SQ_THROW(); }
				TARGET.swap(temp_reg);
				continue;
			case _OP_EQ:{
				bool res;
				if(!isEqual(STK(arg2),COND_LITERAL,res)) { SQ_THROW(); }
				TARGET = res?true:false;
				}continue;
			case _OP_NE:{
				bool res;
				if(!isEqual(STK(arg2),COND_LITERAL,res)) { SQ_THROW(); }
				TARGET = (!res)?true:false;
				} continue;
			case _OP_ADD: _ARITH_(+,TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_SUB: _ARITH_(-,TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_MUL: _ARITH_(*,TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_DIV: _ARITH_NOZERO(/,TARGET,STK(arg2),STK(arg1),"division by zero"); continue;
			case _OP_MOD: ARITH_OP('%',TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_BITW:  _GUARD(BW_OP( arg3,TARGET,STK(arg2),STK(arg1))); continue;
			case _OP_RETURN:
				if((ci)->_generator) {
					(ci)->_generator->kill();
				}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps==0);
					//outres = temp_reg;
					outres.swap(temp_reg);
					return true;
				}
				continue;
			case _OP_LOADNULLS:{ for(int32_t n=0; n < arg1; n++) STK(arg0+n).Null(); }continue;
			case _OP_LOADROOT:  {
				rabbit::WeakRef *w = ci->_closure.toClosure()->_root;
				if(w->_obj.isNull() == false) {
					TARGET = w->_obj;
				} else {
					TARGET = _roottable; //shoud this be like this? or null
				}
								}
				continue;
			case _OP_LOADBOOL: TARGET = arg1?true:false; continue;
			case _OP_DMOVE: STK(arg0) = STK(arg1); STK(arg2) = STK(arg3); continue;
			case _OP_JMP: ci->_ip += (sarg1); continue;
			//case _OP_JNZ: if(!IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
			case _OP_JCMP:
				_GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg0),temp_reg));
				if(IsFalse(temp_reg)) ci->_ip+=(sarg1);
				continue;
			case _OP_JZ: if(IsFalse(STK(arg0))) ci->_ip+=(sarg1); continue;
			case _OP_GETOUTER: {
				rabbit::Closure *cur_cls = ci->_closure.toClosure();
				rabbit::Outer *otr = cur_cls->_outervalues[arg1].toOuter();
				TARGET = *(otr->_valptr);
				}
			continue;
			case _OP_SETOUTER: {
				rabbit::Closure *cur_cls = ci->_closure.toClosure();
				rabbit::Outer   *otr = cur_cls->_outervalues[arg1].toOuter();
				*(otr->_valptr) = STK(arg2);
				if(arg0 != 0xFF) {
					TARGET = STK(arg2);
				}
				}
			continue;
			case _OP_NEWOBJ:
				switch(arg3) {
					case NOT_TABLE: TARGET = rabbit::Table::create(_get_shared_state(this), arg1); continue;
					case NOT_ARRAY: TARGET = rabbit::Array::create(_get_shared_state(this), 0); TARGET.toArray()->reserve(arg1); continue;
					case NOT_CLASS: _GUARD(CLASS_OP(TARGET,arg1,arg2)); continue;
					default: assert(0); continue;
				}
			case _OP_APPENDARRAY:
				{
					rabbit::Object val;
					val._unVal.raw = 0;
				switch(arg2) {
				case AAT_STACK:
					val = STK(arg1); break;
				case AAT_LITERAL:
					val = ci->_literals[arg1]; break;
				case AAT_INT:
					val._type = rabbit::OT_INTEGER;
					val._unVal.nInteger = (int64_t)((int32_t)arg1);
					break;
				case AAT_FLOAT:
					val._type = rabbit::OT_FLOAT;
					val._unVal.fFloat = *((const float_t *)&arg1);
					break;
				case AAT_BOOL:
					val._type = rabbit::OT_BOOL;
					val._unVal.nInteger = arg1;
					break;
				default: val._type = rabbit::OT_INTEGER; assert(0); break;

				}
				STK(arg0).toArray()->append(val); continue;
				}
			case _OP_COMPARITH:
				{
					int64_t selfidx = (((uint64_t)arg1&0xFFFF0000)>>16);
					_GUARD(derefInc(arg3, TARGET, STK(selfidx), STK(arg2), STK(arg1&0x0000FFFF), false, selfidx));
				}
				continue;
			case _OP_INC: {rabbit::ObjectPtr o(sarg3); _GUARD(derefInc('+',TARGET, STK(arg1), STK(arg2), o, false, arg1));} continue;
			case _OP_INCL:
				{
					rabbit::ObjectPtr &a = STK(arg1);
					if(a.isInteger() == true) {
						a._unVal.nInteger = a.toInteger() + sarg3;
					} else {
						rabbit::ObjectPtr o(sarg3); //_GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));
						_ARITH_(+,a,a,o);
					}
				}
				continue;
			case _OP_PINC: {rabbit::ObjectPtr o(sarg3); _GUARD(derefInc('+',TARGET, STK(arg1), STK(arg2), o, true, arg1));} continue;
			case _OP_PINCL:
				{
					rabbit::ObjectPtr &a = STK(arg1);
					if(a.isInteger() == true) {
						TARGET = a;
						a._unVal.nInteger = a.toInteger() + sarg3;
					} else {
						rabbit::ObjectPtr o(sarg3); _GUARD(PLOCAL_INC('+',TARGET, STK(arg1), o));
					}
				}
				continue;
			case _OP_CMP:   _GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg1),TARGET))  continue;
			case _OP_EXISTS: TARGET = get(STK(arg1), STK(arg2), temp_reg, GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_RAW, DONT_FALL_BACK) ? true : false; continue;
			case _OP_INSTANCEOF:
				if(STK(arg1).isClass() == false)
				{raise_error("cannot apply instanceof between a %s and a %s",getTypeName(STK(arg1)),getTypeName(STK(arg2))); SQ_THROW();}
				TARGET = (STK(arg2).isInstance() == true) ? (STK(arg2).toInstance()->instanceOf(STK(arg1).toClass())?true:false) : false;
				continue;
			case _OP_AND:
				if(IsFalse(STK(arg2))) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_OR:
				if(!IsFalse(STK(arg2))) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_NEG: _GUARD(NEG_OP(TARGET,STK(arg1))); continue;
			case _OP_NOT: TARGET = IsFalse(STK(arg1)); continue;
			case _OP_BWNOT:
				if(STK(arg1).isInteger() == true) {
					int64_t t = STK(arg1).toInteger();
					TARGET = int64_t(~t);
					continue;
				}
				raise_error("attempt to perform a bitwise op on a %s", getTypeName(STK(arg1)));
				SQ_THROW();
			case _OP_CLOSURE: {
				rabbit::Closure *c = ci->_closure._unVal.pClosure;
				rabbit::FunctionProto *fp = c->_function;
				if(!CLOSURE_OP(TARGET,fp->_functions[arg1]._unVal.pFunctionProto)) { SQ_THROW(); }
				continue;
			}
			case _OP_YIELD:{
				if(ci->_generator) {
					if(sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
					_GUARD(ci->_generator->yield(this,arg2));
					traps -= ci->_etraps;
					if(sarg1 != MAX_FUNC_STACKSIZE) {
						STK(arg1).swap(temp_reg);
					}
				}
				else { raise_error("trying to yield a '%s',only genenerator can be yielded", getTypeName(ci->_generator)); SQ_THROW();}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps == 0);
					outres = temp_reg;
					return true;
				}

				}
				continue;
			case _OP_RESUME:
				if(STK(arg1).isGenerator() == false) {
					raise_error("trying to resume a '%s',only genenerator can be resumed", getTypeName(STK(arg1)));
					SQ_THROW();
				}
				_GUARD(STK(arg1).toGenerator()->resume(this, TARGET));
				traps += ci->_etraps;
				continue;
			case _OP_FOREACH:{ int tojump;
				_GUARD(FOREACH_OP(STK(arg0),STK(arg2),STK(arg2+1),STK(arg2+2),arg2,sarg1,tojump));
				ci->_ip += tojump; }
				continue;
			case _OP_POSTFOREACH:
				assert(STK(arg0).isGenerator() == true);
				if(STK(arg0).toGenerator()->_state == rabbit::Generator::eDead)
					ci->_ip += (sarg1 - 1);
				continue;
			case _OP_CLONE: _GUARD(clone(STK(arg1), TARGET)); continue;
			case _OP_TYPEOF: _GUARD(typeOf(STK(arg1), TARGET)) continue;
			case _OP_PUSHTRAP:{
				rabbit::Instruction *_iv = ci->_closure.toClosure()->_function->_instructions;
				_etraps.pushBack(rabbit::ExceptionTrap(_top,_stackbase, &_iv[(ci->_ip-_iv)+arg1], arg0)); traps++;
				ci->_etraps++;
							  }
				continue;
			case _OP_POPTRAP: {
				for(int64_t i = 0; i < arg0; i++) {
					_etraps.popBack(); traps--;
					ci->_etraps--;
				}
							  }
				continue;
			case _OP_THROW: raise_error(TARGET); SQ_THROW(); continue;
			case _OP_NEWSLOTA:
				_GUARD(newSlotA(STK(arg1),STK(arg2),STK(arg3),(arg0&NEW_SLOT_ATTRIBUTES_FLAG) ? STK(arg2-1) : rabbit::ObjectPtr(),(arg0&NEW_SLOT_STATIC_FLAG)?true:false,false));
				continue;
			case _OP_GETBASE:{
				rabbit::Closure *clo = ci->_closure.toClosure();
				if(clo->_base) {
					TARGET = clo->_base;
				}
				else {
					TARGET.Null();
				}
				continue;
			}
			case _OP_CLOSE:
				if(_openouters) closeOuters(&(STK(arg1)));
				continue;
			}

		}
	}
exception_trap:
	{
		rabbit::ObjectPtr currerror = _lasterror;
//	  dumpstack(_stackbase);
//	  int64_t n = 0;
		int64_t last_top = _top;

		if(_get_shared_state(this)->_notifyallexceptions || (!traps && raiseerror)) callerrorHandler(currerror);

		while( ci ) {
			if(ci->_etraps > 0) {
				rabbit::ExceptionTrap &et = _etraps.back();
				ci->_ip = et._ip;
				_top = et._stacksize;
				_stackbase = et._stackbase;
				_stack[_stackbase + et._extarget] = currerror;
				_etraps.popBack(); traps--; ci->_etraps--;
				while(last_top >= _top) _stack[last_top--].Null();
				goto exception_restore;
			}
			else if (_debughook) {
					//notify debugger of a "return"
					//even if it really an exception unwinding the stack
					for(int64_t i = 0; i < ci->_ncalls; i++) {
						callDebugHook('r');
					}
			}
			if(ci->_generator) ci->_generator->kill();
			bool mustbreak = ci && ci->_root;
			leaveFrame();
			if(mustbreak) break;
		}

		_lasterror = currerror;
		return false;
	}
	assert(0);
}

bool rabbit::VirtualMachine::createClassInstance(rabbit::Class *theclass, rabbit::ObjectPtr &inst, rabbit::ObjectPtr &constructor)
{
	inst = theclass->createInstance();
	if(!theclass->getConstructor(constructor)) {
		constructor.Null();
	}
	return true;
}

void rabbit::VirtualMachine::callerrorHandler(rabbit::ObjectPtr &error)
{
	if(_errorhandler.isNull() == false) {
		rabbit::ObjectPtr out;
		push(_roottable); push(error);
		call(_errorhandler, 2, _top-2, out,SQFalse);
		pop(2);
	}
}


void rabbit::VirtualMachine::callDebugHook(int64_t type,int64_t forcedline)
{
	_debughook = false;
	rabbit::FunctionProto *func=ci->_closure.toClosure()->_function;
	if(_debughook_native) {
		const char* src = nullptr;
		if (func->_sourcename.isString() == true) {
			src = func->_sourcename.getStringValue();
		}
		const char* fname = nullptr;
		if (func->_name.isString() == true) {
			fname = func->_name.getStringValue();
		}
		int64_t line = forcedline?forcedline:func->getLine(ci->_ip);
		_debughook_native(this,type,src,line,fname);
	}
	else {
		rabbit::ObjectPtr temp_reg;
		int64_t nparams=5;
		push(_roottable);
		push(type);
		push(func->_sourcename);
		push(forcedline?forcedline:func->getLine(ci->_ip));
		push(func->_name);
		call(_debughook_closure,nparams,_top-nparams,temp_reg,SQFalse);
		pop(nparams);
	}
	_debughook = true;
}

bool rabbit::VirtualMachine::callNative(rabbit::NativeClosure *nclosure, int64_t nargs, int64_t newbase, rabbit::ObjectPtr &retval, int32_t target,bool &suspend, bool &tailcall)
{
	int64_t nparamscheck = nclosure->_nparamscheck;
	int64_t newtop = newbase + nargs + nclosure->_noutervalues;

	if (_nnativecalls + 1 > MAX_NATIVE_CALLS) {
		raise_error("Native stack overflow");
		return false;
	}

	if(nparamscheck && (((nparamscheck > 0) && (nparamscheck != nargs)) ||
		((nparamscheck < 0) && (nargs < (-nparamscheck)))))
	{
		raise_error("wrong number of parameters");
		return false;
	}

	int64_t tcs;
	etk::Vector<int64_t> &tc = nclosure->_typecheck;
	if((tcs = tc.size())) {
		for(int64_t i = 0; i < nargs && i < tcs; i++) {
			if((tc[i] != -1) && !(_stack[newbase+i].getType() & tc[i])) {
				raise_ParamTypeerror(i,tc[i], _stack[newbase+i].getType());
				return false;
			}
		}
	}

	if(!enterFrame(newbase, newtop, false)) return false;
	ci->_closure  = nclosure;
	ci->_target = target;

	int64_t outers = nclosure->_noutervalues;
	for (int64_t i = 0; i < outers; i++) {
		_stack[newbase+nargs+i] = nclosure->_outervalues[i];
	}
	if(nclosure->_env) {
		_stack[newbase] = nclosure->_env->_obj;
	}

	_nnativecalls++;
	int64_t ret = (nclosure->_function)(this);
	_nnativecalls--;

	suspend = false;
	tailcall = false;
	if (ret == SQ_TAILCALL_FLAG) {
		tailcall = true;
		return true;
	}
	else if (ret == SQ_SUSPEND_FLAG) {
		suspend = true;
	}
	else if (ret < 0) {
		leaveFrame();
		raise_error(_lasterror);
		return false;
	}
	if(ret) {
		retval = _stack[_top-1];
	}
	else {
		retval.Null();
	}
	//retval = ret ? _stack[_top-1] : _null_;
	leaveFrame();
	return true;
}

bool rabbit::VirtualMachine::tailcall(rabbit::Closure *closure, int64_t parambase,int64_t nparams)
{
	int64_t last_top = _top;
	rabbit::ObjectPtr clo = closure;
	if (ci->_root)
	{
		raise_error("root calls cannot invoke tailcalls");
		return false;
	}
	for (int64_t i = 0; i < nparams; i++) STK(i) = STK(parambase + i);
	bool ret = startcall(closure, ci->_target, nparams, _stackbase, true);
	if (last_top >= _top) {
		_top = last_top;
	}
	return ret;
}

#define FALLBACK_OK        0
#define FALLBACK_NO_MATCH  1
#define FALLBACK_ERROR     2

bool rabbit::VirtualMachine::get(const rabbit::ObjectPtr &self, const rabbit::ObjectPtr &key, rabbit::ObjectPtr &dest, uint64_t getflags, int64_t selfidx) {
	switch(self.getType()){
		case rabbit::OT_TABLE:
			if(self.toTable()->get(key,dest)) {
				return true;
			}
			break;
		case rabbit::OT_ARRAY:
			if (key.isNumeric() == true) {
				if (self.toArray()->get(key.toIntegerValue(), dest)) {
					return true;
				}
				if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) {
					raise_Idxerror(key);
				}
				return false;
			}
			break;
		case rabbit::OT_INSTANCE:
			if(const_cast<rabbit::Instance*>(self.toInstance())->get(key,dest)) {
				return true;
			}
			break;
		case rabbit::OT_CLASS:
			if(const_cast<rabbit::Class*>(self.toClass())->get(key,dest)) {
				return true;
			}
			break;
		case rabbit::OT_STRING:
			if(key.isNumeric()){
				int64_t n = key.toIntegerValue();
				int64_t len = self.toString()->_len;
				if (n < 0) { n += len; }
				if (n >= 0 && n < len) {
					dest = int64_t(self.getStringValue()[n]);
					return true;
				}
				if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) raise_Idxerror(key);
				return false;
			}
			break;
		default:
			break; //shut up compiler
	}
	if ((getflags & GET_FLAG_RAW) == 0) {
		switch(fallBackGet(self,key,dest)) {
			case FALLBACK_OK: 
				return true; //okie
			case FALLBACK_NO_MATCH:
				break; //keep falling back
			case FALLBACK_ERROR:
				return false; // the metamethod failed
		}
		if(invokeDefaultDelegate(self,key,dest)) {
			return true;
		}
	}
//#ifdef ROrabbit::OT_FALLBACK
	if(selfidx == 0) {
		rabbit::WeakRef *w = ci->_closure.toClosure()->_root;
		if(w->_obj.isNull() == false) {
			if(get(*((const rabbit::ObjectPtr *)&w->_obj),key,dest,0,DONT_FALL_BACK)) {
				return true;
			}
		}
	}
//#endif
	if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) raise_Idxerror(key);
	return false;
}

bool rabbit::VirtualMachine::invokeDefaultDelegate(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,rabbit::ObjectPtr &dest)
{
	rabbit::Table *ddel = NULL;
	switch(self.getType()) {
		case rabbit::OT_CLASS: ddel = _class_ddel; break;
		case rabbit::OT_TABLE: ddel = _table_ddel; break;
		case rabbit::OT_ARRAY: ddel = _array_ddel; break;
		case rabbit::OT_STRING: ddel = _string_ddel; break;
		case rabbit::OT_INSTANCE: ddel = _instance_ddel; break;
		case rabbit::OT_INTEGER:case OT_FLOAT:case OT_BOOL: ddel = _number_ddel; break;
		case rabbit::OT_GENERATOR: ddel = _generator_ddel; break;
		case rabbit::OT_CLOSURE: case OT_NATIVECLOSURE: ddel = _closure_ddel; break;
		case rabbit::OT_THREAD: ddel = _thread_ddel; break;
		case rabbit::OT_WEAKREF: ddel = _weakref_ddel; break;
		default: return false;
	}
	return  ddel->get(key,dest);
}


int64_t rabbit::VirtualMachine::fallBackGet(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,rabbit::ObjectPtr &dest)
{
	switch(self.getType()){
	case rabbit::OT_TABLE:
	case rabbit::OT_USERDATA:
		//delegation
		if(self.toDelegable()->_delegate) {
			if(get(rabbit::ObjectPtr(self.toDelegable()->_delegate),key,dest,0,DONT_FALL_BACK)) return FALLBACK_OK;
		}
		else {
			return FALLBACK_NO_MATCH;
		}
		//go through
	case rabbit::OT_INSTANCE: {
		rabbit::ObjectPtr closure;
		if(self.toDelegable()->getMetaMethod(this, MT_GET, closure)) {
			push(self);push(key);
			_nmetamethodscall++;
			AutoDec ad(&_nmetamethodscall);
			if(call(closure, 2, _top - 2, dest, SQFalse)) {
				pop(2);
				return FALLBACK_OK;
			}
			else {
				pop(2);
				if(_lasterror.isNull() == false) {
					//NULL means "clean failure" (not found)
					return FALLBACK_ERROR;
				}
			}
		}
		}
		break;
	default: break;//shutup GCC 4.x
	}
	// no metamethod or no fallback type
	return FALLBACK_NO_MATCH;
}

bool rabbit::VirtualMachine::set(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,int64_t selfidx)
{
	switch(self.getType()){
	case rabbit::OT_TABLE:
		if(self.toTable()->set(key,val)) return true;
		break;
	case rabbit::OT_INSTANCE:
		if(const_cast<rabbit::Instance*>(self.toInstance())->set(key,val)) return true;
		break;
	case rabbit::OT_ARRAY:
		if(key.isNumeric() == false) { raise_error("indexing %s with %s",getTypeName(self),getTypeName(key)); return false; }
		if(!self.toArray()->set(key.toIntegerValue(),val)) {
			raise_Idxerror(key);
			return false;
		}
		return true;
	case rabbit::OT_USERDATA: break; // must fall back
	default:
		raise_error("trying to set '%s'",getTypeName(self));
		return false;
	}

	switch(fallBackSet(self,key,val)) {
		case FALLBACK_OK: return true; //okie
		case FALLBACK_NO_MATCH: break; //keep falling back
		case FALLBACK_ERROR: return false; // the metamethod failed
	}
	if(selfidx == 0) {
		if(_roottable.toTable()->set(key,val))
			return true;
	}
	raise_Idxerror(key);
	return false;
}

int64_t rabbit::VirtualMachine::fallBackSet(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val)
{
	switch(self.getType()) {
	case rabbit::OT_TABLE:
		if(self.toTable()->_delegate) {
			if(set(self.toTable()->_delegate,key,val,DONT_FALL_BACK)) return FALLBACK_OK;
		}
		//keps on going
	case rabbit::OT_INSTANCE:
	case rabbit::OT_USERDATA:{
		rabbit::ObjectPtr closure;
		rabbit::ObjectPtr t;
		if(self.toDelegable()->getMetaMethod(this, MT_SET, closure)) {
			push(self);push(key);push(val);
			_nmetamethodscall++;
			AutoDec ad(&_nmetamethodscall);
			if(call(closure, 3, _top - 3, t, SQFalse)) {
				pop(3);
				return FALLBACK_OK;
			}
			else {
				pop(3);
				if(_lasterror.isNull() == false) {
					//NULL means "clean failure" (not found)
					return FALLBACK_ERROR;
				}
			}
		}
		}
		break;
		default: break;//shutup GCC 4.x
	}
	// no metamethod or no fallback type
	return FALLBACK_NO_MATCH;
}

bool rabbit::VirtualMachine::clone(const rabbit::ObjectPtr &self,rabbit::ObjectPtr &target)
{
	rabbit::ObjectPtr temp_reg;
	rabbit::ObjectPtr newobj;
	switch(self.getType()){
	case rabbit::OT_TABLE:
		newobj = self.toTable()->clone();
		goto cloned_mt;
	case rabbit::OT_INSTANCE: {
		newobj = const_cast<rabbit::Instance*>(self.toInstance())->clone(_get_shared_state(this));
cloned_mt:
		rabbit::ObjectPtr closure;
		if(newobj.toDelegable()->_delegate && newobj.toDelegable()->getMetaMethod(this,MT_CLONED,closure)) {
			push(newobj);
			push(self);
			if(!callMetaMethod(closure,MT_CLONED,2,temp_reg))
				return false;
		}
		}
		target = newobj;
		return true;
	case rabbit::OT_ARRAY:
		target = self.toArray()->clone();
		return true;
	default:
		raise_error("cloning a %s", getTypeName(self));
		return false;
	}
}

bool rabbit::VirtualMachine::newSlotA(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,const rabbit::ObjectPtr &attrs,bool bstatic,bool raw)
{
	if(self.isClass() == false) {
		raise_error("object must be a class");
		return false;
	}
	rabbit::Class *c = const_cast<rabbit::Class*>(self.toClass());
	if(!raw) {
		rabbit::ObjectPtr &mm = c->_metamethods[MT_NEWMEMBER];
		if(mm.isNull() == false ) {
			push(self);
			push(key);
			push(val);
			push(attrs);
			push(bstatic);
			return callMetaMethod(mm,MT_NEWMEMBER,5,temp_reg);
		}
	}
	if(!newSlot(self, key, val,bstatic))
		return false;
	if(attrs.isNull() == false) {
		c->setAttributes(key,attrs);
	}
	return true;
}

bool rabbit::VirtualMachine::newSlot(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,bool bstatic)
{
	if(key.isNull() == true) {
		raise_error("null cannot be used as index");
		return false;
	}
	switch(self.getType()) {
	case rabbit::OT_TABLE:
		{
			bool rawcall = true;
			if(self.toTable()->_delegate) {
				rabbit::ObjectPtr res;
				if(!self.toTable()->get(key,res)) {
					rabbit::ObjectPtr closure;
					if(self.toDelegable()->_delegate && self.toDelegable()->getMetaMethod(this,MT_NEWSLOT,closure)) {
						push(self);push(key);push(val);
						if(!callMetaMethod(closure,MT_NEWSLOT,3,res)) {
							return false;
						}
						rawcall = false;
					}
					else {
						rawcall = true;
					}
				}
			}
			if(rawcall) {
				self.toTable()->newSlot(key,val); //cannot fail
			}
			break;
		}
	case rabbit::OT_INSTANCE: {
		rabbit::ObjectPtr res;
		rabbit::ObjectPtr closure;
		if(self.toDelegable()->_delegate && self.toDelegable()->getMetaMethod(this,MT_NEWSLOT,closure)) {
			push(self);push(key);push(val);
			if(!callMetaMethod(closure,MT_NEWSLOT,3,res)) {
				return false;
			}
			break;
		}
		raise_error("class instances do not support the new slot operator");
		return false;
		break;}
	case rabbit::OT_CLASS:
		if(!const_cast<rabbit::Class*>(self.toClass())->newSlot(_get_shared_state(this),key,val,bstatic)) {
			if(self.toClass()->_locked) {
				raise_error("trying to modify a class that has already been instantiated");
				return false;
			}
			else {
				rabbit::ObjectPtr oval = printObjVal(key);
				raise_error("the property '%s' already exists",oval.getStringValue());
				return false;
			}
		}
		break;
	default:
		raise_error("indexing %s with %s",getTypeName(self),getTypeName(key));
		return false;
		break;
	}
	return true;
}



bool rabbit::VirtualMachine::deleteSlot(const rabbit::ObjectPtr &self,const rabbit::ObjectPtr &key,rabbit::ObjectPtr &res)
{
	switch(self.getType()) {
	case rabbit::OT_TABLE:
	case rabbit::OT_INSTANCE:
	case rabbit::OT_USERDATA: {
		rabbit::ObjectPtr t;
		//bool handled = false;
		rabbit::ObjectPtr closure;
		if(self.toDelegable()->_delegate && self.toDelegable()->getMetaMethod(this,MT_DELSLOT,closure)) {
			push(self);push(key);
			return callMetaMethod(closure,MT_DELSLOT,2,res);
		}
		else {
			if(self.isTable() == true) {
				if(self.toTable()->get(key,t)) {
					self.toTable()->remove(key);
				}
				else {
					raise_Idxerror((const rabbit::Object &)key);
					return false;
				}
			}
			else {
				raise_error("cannot delete a slot from %s",getTypeName(self));
				return false;
			}
		}
		res = t;
				}
		break;
	default:
		raise_error("attempt to delete a slot from a %s",getTypeName(self));
		return false;
	}
	return true;
}

bool rabbit::VirtualMachine::call(rabbit::ObjectPtr &closure,int64_t nparams,int64_t stackbase,rabbit::ObjectPtr &outres,rabbit::Bool raiseerror)
{
#ifdef _DEBUG
int64_t prevstackbase = _stackbase;
#endif
	switch(closure.getType()) {
	case rabbit::OT_CLOSURE:
		return execute(closure, nparams, stackbase, outres, raiseerror);
		break;
	case rabbit::OT_NATIVECLOSURE:{
		bool dummy;
		return callNative(closure.toNativeClosure(), nparams, stackbase, outres, -1, dummy, dummy);

						  }
		break;
	case rabbit::OT_CLASS: {
		rabbit::ObjectPtr constr;
		rabbit::ObjectPtr temp;
		createClassInstance(closure.toClass(),outres,constr);
		rabbit::ObjectType ctype = constr.getType();
		if (ctype == rabbit::OT_NATIVECLOSURE || ctype == OT_CLOSURE) {
			_stack[stackbase] = outres;
			return call(constr,nparams,stackbase,temp,raiseerror);
		}
		return true;
				   }
		break;
	default:
		return false;
	}
#ifdef _DEBUG
	if(!_suspended) {
		assert(_stackbase == prevstackbase);
	}
#endif
	return true;
}

bool rabbit::VirtualMachine::callMetaMethod(rabbit::ObjectPtr &closure,rabbit::MetaMethod SQ_UNUSED_ARG(mm),int64_t nparams,rabbit::ObjectPtr &outres)
{
	//rabbit::ObjectPtr closure;

	_nmetamethodscall++;
	if(call(closure, nparams, _top - nparams, outres, SQFalse)) {
		_nmetamethodscall--;
		pop(nparams);
		return true;
	}
	_nmetamethodscall--;
	//}
	pop(nparams);
	return false;
}

void rabbit::VirtualMachine::findOuter(rabbit::ObjectPtr &target, rabbit::ObjectPtr *stackindex)
{
	rabbit::Outer **pp = &_openouters;
	rabbit::Outer *p;
	rabbit::Outer *otr;

	while ((p = *pp) != NULL && p->_valptr >= stackindex) {
		if (p->_valptr == stackindex) {
			target = rabbit::ObjectPtr(p);
			return;
		}
		pp = &p->_next;
	}
	otr = rabbit::Outer::create(_get_shared_state(this), stackindex);
	otr->_next = *pp;
	// TODO: rework this, this is absolutly not safe...
	otr->_idx  = (stackindex - &_stack[0]);
	__ObjaddRef(otr);
	*pp = otr;
	target = rabbit::ObjectPtr(otr);
}

bool rabbit::VirtualMachine::enterFrame(int64_t newbase, int64_t newtop, bool tailcall)
{
	if( !tailcall ) {
		if( _callsstacksize == _alloccallsstacksize ) {
			GrowcallStack();
		}
		ci = &_callsstack[_callsstacksize++];
		ci->_prevstkbase = (int32_t)(newbase - _stackbase);
		ci->_prevtop = (int32_t)(_top - _stackbase);
		ci->_etraps = 0;
		ci->_ncalls = 1;
		ci->_generator = NULL;
		ci->_root = SQFalse;
	}
	else {
		ci->_ncalls++;
	}

	_stackbase = newbase;
	_top = newtop;
	if(newtop + MIN_STACK_OVERHEAD > (int64_t)_stack.size()) {
		if(_nmetamethodscall) {
			raise_error("stack overflow, cannot resize stack while in a metamethod");
			return false;
		}
		_stack.resize(newtop + (MIN_STACK_OVERHEAD << 2));
		relocateOuters();
	}
	return true;
}

void rabbit::VirtualMachine::leaveFrame() {
	int64_t last_top = _top;
	int64_t last_stackbase = _stackbase;
	int64_t css = --_callsstacksize;

	/* First clean out the call stack frame */
	ci->_closure.Null();
	_stackbase -= ci->_prevstkbase;
	_top = _stackbase + ci->_prevtop;
	ci = (css) ? &_callsstack[css-1] : NULL;

	if(_openouters) closeOuters(&(_stack[last_stackbase]));
	while (last_top >= _top) {
		_stack[last_top--].Null();
	}
}

void rabbit::VirtualMachine::relocateOuters()
{
	rabbit::Outer *p = _openouters;
	while (p) {
		p->_valptr = &_stack[p->_idx];
		p = p->_next;
	}
}

void rabbit::VirtualMachine::closeOuters(rabbit::ObjectPtr *stackindex) {
  rabbit::Outer *p;
  while ((p = _openouters) != NULL && p->_valptr >= stackindex) {
	p->_value = *(p->_valptr);
	p->_valptr = &p->_value;
	_openouters = p->_next;
	__Objrelease(p);
  }
}

void rabbit::VirtualMachine::remove(int64_t n) {
	n = (n >= 0)?n + _stackbase - 1:_top + n;
	for(int64_t i = n; i < _top; i++){
		_stack[i] = _stack[i+1];
	}
	_stack[_top].Null();
	_top--;
}

void rabbit::VirtualMachine::pop() {
	_stack[--_top].Null();
}

void rabbit::VirtualMachine::pop(int64_t n) {
	for(int64_t i = 0; i < n; i++){
		_stack[--_top].Null();
	}
}

void rabbit::VirtualMachine::pushNull() { _stack[_top++].Null(); }
void rabbit::VirtualMachine::push(const rabbit::ObjectPtr &o) { _stack[_top++] = o; }
rabbit::ObjectPtr &rabbit::VirtualMachine::top() { return _stack[_top-1]; }
rabbit::ObjectPtr &rabbit::VirtualMachine::popGet() { return _stack[--_top]; }
rabbit::ObjectPtr &rabbit::VirtualMachine::getUp(int64_t n) { return _stack[_top+n]; }
rabbit::ObjectPtr &rabbit::VirtualMachine::getAt(int64_t n) { return _stack[n]; }

#ifdef _DEBUG_DUMP
void rabbit::VirtualMachine::dumpstack(int64_t stackbase,bool dumpall)
{
	int64_t size=dumpall?_stack.size():_top;
	int64_t n=0;
	printf("\n>>>>stack dump<<<<\n");
	callInfo &ci=_callsstack[_callsstacksize-1];
	printf("IP: %p\n",ci._ip);
	printf("prev stack base: %d\n",ci._prevstkbase);
	printf("prev top: %d\n",ci._prevtop);
	for(int64_t i=0;i<size;i++){
		rabbit::ObjectPtr &obj=_stack[i];
		if(stackbase==i) {
			printf(">");
		} else {
			printf(" ");
		}
		printf("[" _PRINT_INT_FMT "]:",n);
		switch(obj.getType()){
		case rabbit::OT_FLOAT:		  printf("FLOAT %.3f",obj.toFloat());break;
		case rabbit::OT_INTEGER:		printf("INTEGER " _PRINT_INT_FMT,obj.toInteger());break;
		case rabbit::OT_BOOL:		   printf("BOOL %s",obj.toInteger()?"true":"false");break;
		case rabbit::OT_STRING:		 printf("STRING %s",obj.getStringValue());break;
		case rabbit::OT_NULL:		   printf("NULL");  break;
		case rabbit::OT_TABLE:		  printf("TABLE %p[%p]",obj.toTable(),obj.toTable()->_delegate);break;
		case rabbit::OT_ARRAY:		  printf("ARRAY %p",obj.toArray());break;
		case rabbit::OT_CLOSURE:		printf("CLOSURE [%p]",obj.toClosure());break;
		case rabbit::OT_NATIVECLOSURE:  printf("NATIVECLOSURE");break;
		case rabbit::OT_USERDATA:	   printf("USERDATA %p[%p]", obj.getUserDataValue(), obj.toUserData()->_delegate);break;
		case rabbit::OT_GENERATOR:	  printf("GENERATOR %p",obj.toGenerator());break;
		case rabbit::OT_THREAD:		 printf("THREAD [%p]",obj.toVirtualMachine());break;
		case rabbit::OT_USERPOINTER:	printf("USERPOINTER %p",obj.toUserPointer());break;
		case rabbit::OT_CLASS:		  printf("CLASS %p",obj.toClass());break;
		case rabbit::OT_INSTANCE:	   printf("INSTANCE %p",obj.toInstance());break;
		case rabbit::OT_WEAKREF:		printf("WEAKERF %p",obj.toWeakRef());break;
		default:
			assert(0);
			break;
		};
		printf("\n");
		++n;
	}
}



#endif
