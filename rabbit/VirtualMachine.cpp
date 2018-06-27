/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/sqpcheader.hpp>
#include <math.h>
#include <stdlib.h>
#include <rabbit/sqopcodes.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/sqstring.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/UserData.hpp>
#include <rabbit/Array.hpp>
#include <rabbit/sqclass.hpp>

#define TOP() (_stack[_top-1])
#define TARGET _stack[_stackbase+arg0]
#define STK(a) _stack[_stackbase+(a)]

bool rabbit::VirtualMachine::BW_OP(uint64_t op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2)
{
	int64_t res;
	if((sq_type(o1)| sq_type(o2)) == OT_INTEGER)
	{
		int64_t i1 = _integer(o1), i2 = _integer(o2);
		switch(op) {
			case BW_AND:	res = i1 & i2; break;
			case BW_OR:	 res = i1 | i2; break;
			case BW_XOR:	res = i1 ^ i2; break;
			case BW_SHIFTL: res = i1 << i2; break;
			case BW_SHIFTR: res = i1 >> i2; break;
			case BW_USHIFTR:res = (int64_t)(*((uint64_t*)&i1) >> i2); break;
			default: { raise_error(_SC("internal vm error bitwise op failed")); return false; }
		}
	}
	else { raise_error(_SC("bitwise op between '%s' and '%s'"),getTypeName(o1),getTypeName(o2)); return false;}
	trg = res;
	return true;
}

#define _ARITH_(op,trg,o1,o2) \
{ \
	int64_t tmask = sq_type(o1)|sq_type(o2); \
	switch(tmask) { \
		case OT_INTEGER: trg = _integer(o1) op _integer(o2);break; \
		case (OT_FLOAT|OT_INTEGER): \
		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

#define _ARITH_NOZERO(op,trg,o1,o2,err) \
{ \
	int64_t tmask = sq_type(o1)|sq_type(o2); \
	switch(tmask) { \
		case OT_INTEGER: { int64_t i2 = _integer(o2); if(i2 == 0) { raise_error(err); SQ_THROW(); } trg = _integer(o1) op i2; } break;\
		case (OT_FLOAT|OT_INTEGER): \
		case (OT_FLOAT): trg = tofloat(o1) op tofloat(o2); break;\
		default: _GUARD(ARITH_OP((#op)[0],trg,o1,o2)); break;\
	} \
}

bool rabbit::VirtualMachine::ARITH_OP(uint64_t op,SQObjectPtr &trg,const SQObjectPtr &o1,const SQObjectPtr &o2)
{
	int64_t tmask = sq_type(o1)| sq_type(o2);
	switch(tmask) {
		case OT_INTEGER:{
			int64_t res, i1 = _integer(o1), i2 = _integer(o2);
			switch(op) {
			case '+': res = i1 + i2; break;
			case '-': res = i1 - i2; break;
			case '/': if (i2 == 0) { raise_error(_SC("division by zero")); return false; }
					else if (i2 == -1 && i1 == INT_MIN) { raise_error(_SC("integer overflow")); return false; }
					res = i1 / i2;
					break;
			case '*': res = i1 * i2; break;
			case '%': if (i2 == 0) { raise_error(_SC("modulo by zero")); return false; }
					else if (i2 == -1 && i1 == INT_MIN) { res = 0; break; }
					res = i1 % i2;
					break;
			default: res = 0xDEADBEEF;
			}
			trg = res; }
			break;
		case (OT_FLOAT|OT_INTEGER):
		case (OT_FLOAT):{
			float_t res, f1 = tofloat(o1), f2 = tofloat(o2);
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

rabbit::VirtualMachine::VirtualMachine(SQSharedState *ss)
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

bool rabbit::VirtualMachine::arithMetaMethod(int64_t op,const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest)
{
	SQMetaMethod mm;
	switch(op){
		case _SC('+'): mm=MT_ADD; break;
		case _SC('-'): mm=MT_SUB; break;
		case _SC('/'): mm=MT_DIV; break;
		case _SC('*'): mm=MT_MUL; break;
		case _SC('%'): mm=MT_MODULO; break;
		default: mm = MT_ADD; assert(0); break; //shutup compiler
	}
	if(is_delegable(o1) && _delegable(o1)->_delegate) {

		SQObjectPtr closure;
		if(_delegable(o1)->getMetaMethod(this, mm, closure)) {
			push(o1);
			push(o2);
			return callMetaMethod(closure,mm,2,dest);
		}
	}
	raise_error(_SC("arith op %c on between '%s' and '%s'"),op,getTypeName(o1),getTypeName(o2));
	return false;
}

bool rabbit::VirtualMachine::NEG_OP(SQObjectPtr &trg,const SQObjectPtr &o)
{

	switch(sq_type(o)) {
	case OT_INTEGER:
		trg = -_integer(o);
		return true;
	case OT_FLOAT:
		trg = -_float(o);
		return true;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o)->_delegate) {
			SQObjectPtr closure;
			if(_delegable(o)->getMetaMethod(this, MT_UNM, closure)) {
				push(o);
				if(!callMetaMethod(closure, MT_UNM, 1, temp_reg)) return false;
				_Swap(trg,temp_reg);
				return true;

			}
		}
	default:break; //shutup compiler
	}
	raise_error(_SC("attempt to negate a %s"), getTypeName(o));
	return false;
}

#define _RET_SUCCEED(exp) { result = (exp); return true; }
bool rabbit::VirtualMachine::objCmp(const SQObjectPtr &o1,const SQObjectPtr &o2,int64_t &result)
{
	SQObjectType t1 = sq_type(o1), t2 = sq_type(o2);
	if(t1 == t2) {
		if(_rawval(o1) == _rawval(o2))_RET_SUCCEED(0);
		SQObjectPtr res;
		switch(t1){
		case OT_STRING:
			_RET_SUCCEED(scstrcmp(_stringval(o1),_stringval(o2)));
		case OT_INTEGER:
			_RET_SUCCEED((_integer(o1)<_integer(o2))?-1:1);
		case OT_FLOAT:
			_RET_SUCCEED((_float(o1)<_float(o2))?-1:1);
		case OT_TABLE:
		case OT_USERDATA:
		case OT_INSTANCE:
			if(_delegable(o1)->_delegate) {
				SQObjectPtr closure;
				if(_delegable(o1)->getMetaMethod(this, MT_CMP, closure)) {
					push(o1);push(o2);
					if(callMetaMethod(closure,MT_CMP,2,res)) {
						if(sq_type(res) != OT_INTEGER) {
							raise_error(_SC("_cmp must return an integer"));
							return false;
						}
						_RET_SUCCEED(_integer(res))
					}
					return false;
				}
			}
			//continues through (no break needed)
		default:
			_RET_SUCCEED( _userpointer(o1) < _userpointer(o2)?-1:1 );
		}
		assert(0);
		//if(type(res)!=OT_INTEGER) { raise_Compareerror(o1,o2); return false; }
		//  _RET_SUCCEED(_integer(res));

	}
	else{
		if(sq_isnumeric(o1) && sq_isnumeric(o2)){
			if((t1==OT_INTEGER) && (t2==OT_FLOAT)) {
				if( _integer(o1)==_float(o2) ) { _RET_SUCCEED(0); }
				else if( _integer(o1)<_float(o2) ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
			else{
				if( _float(o1)==_integer(o2) ) { _RET_SUCCEED(0); }
				else if( _float(o1)<_integer(o2) ) { _RET_SUCCEED(-1); }
				_RET_SUCCEED(1);
			}
		}
		else if(t1==OT_NULL) {_RET_SUCCEED(-1);}
		else if(t2==OT_NULL) {_RET_SUCCEED(1);}
		else { raise_Compareerror(o1,o2); return false; }

	}
	assert(0);
	_RET_SUCCEED(0); //cannot happen
}

bool rabbit::VirtualMachine::CMP_OP(CmpOP op, const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &res)
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

bool rabbit::VirtualMachine::toString(const SQObjectPtr &o,SQObjectPtr &res)
{
	switch(sq_type(o)) {
	case OT_STRING:
		res = o;
		return true;
	case OT_FLOAT:
		scsprintf(_sp(sq_rsl(NUMBER_MAX_CHAR+1)),sq_rsl(NUMBER_MAX_CHAR),_SC("%g"),_float(o));
		break;
	case OT_INTEGER:
		scsprintf(_sp(sq_rsl(NUMBER_MAX_CHAR+1)),sq_rsl(NUMBER_MAX_CHAR),_PRINT_INT_FMT,_integer(o));
		break;
	case OT_BOOL:
		scsprintf(_sp(sq_rsl(6)),sq_rsl(6),_integer(o)?_SC("true"):_SC("false"));
		break;
	case OT_TABLE:
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o)->_delegate) {
			SQObjectPtr closure;
			if(_delegable(o)->getMetaMethod(this, MT_TOSTRING, closure)) {
				push(o);
				if(callMetaMethod(closure,MT_TOSTRING,1,res)) {
					if(sq_type(res) == OT_STRING)
						return true;
				}
				else {
					return false;
				}
			}
		}
	default:
		scsprintf(_sp(sq_rsl((sizeof(void*)*2)+NUMBER_MAX_CHAR)),sq_rsl((sizeof(void*)*2)+NUMBER_MAX_CHAR),_SC("(%s : 0x%p)"),getTypeName(o),(void*)_rawval(o));
	}
	res = SQString::create(_ss(this),_spval);
	return true;
}


bool rabbit::VirtualMachine::stringCat(const SQObjectPtr &str,const SQObjectPtr &obj,SQObjectPtr &dest)
{
	SQObjectPtr a, b;
	if(!toString(str, a)) return false;
	if(!toString(obj, b)) return false;
	int64_t l = _string(a)->_len , ol = _string(b)->_len;
	SQChar *s = _sp(sq_rsl(l + ol + 1));
	memcpy(s, _stringval(a), sq_rsl(l));
	memcpy(s + l, _stringval(b), sq_rsl(ol));
	dest = SQString::create(_ss(this), _spval, l + ol);
	return true;
}

bool rabbit::VirtualMachine::typeOf(const SQObjectPtr &obj1,SQObjectPtr &dest)
{
	if(is_delegable(obj1) && _delegable(obj1)->_delegate) {
		SQObjectPtr closure;
		if(_delegable(obj1)->getMetaMethod(this, MT_TYPEOF, closure)) {
			push(obj1);
			return callMetaMethod(closure,MT_TYPEOF,1,dest);
		}
	}
	dest = SQString::create(_ss(this),getTypeName(obj1));
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
		_roottable = SQTable::create(_ss(this), 0);
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


bool rabbit::VirtualMachine::startcall(SQClosure *closure,int64_t target,int64_t args,int64_t stackbase,bool tailcall)
{
	SQFunctionProto *func = closure->_function;

	int64_t paramssize = func->_nparameters;
	const int64_t newtop = stackbase + func->_stacksize;
	int64_t nargs = args;
	if(func->_varparams)
	{
		paramssize--;
		if (nargs < paramssize) {
			raise_error(_SC("wrong number of parameters"));
			return false;
		}

		//dumpstack(stackbase);
		int64_t nvargs = nargs - paramssize;
		rabbit::Array *arr = rabbit::Array::create(_ss(this),nvargs);
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
			raise_error(_SC("wrong number of parameters"));
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
		callDebugHook(_SC('c'));
	}

	if (closure->_function->_bgenerator) {
		SQFunctionProto *f = closure->_function;
		SQGenerator *gen = SQGenerator::create(_ss(this), closure);
		if(!gen->yield(this,f->_stacksize))
			return false;
		SQObjectPtr temp;
		Return(1, target, temp);
		STK(target) = gen;
	}


	return true;
}

bool rabbit::VirtualMachine::Return(int64_t _arg0, int64_t _arg1, SQObjectPtr &retval)
{
	SQBool	_isroot	  = ci->_root;
	int64_t callerbase   = _stackbase - ci->_prevstkbase;

	if (_debughook) {
		for(int64_t i=0; i<ci->_ncalls; i++) {
			callDebugHook(_SC('r'));
		}
	}

	SQObjectPtr *dest;
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

bool rabbit::VirtualMachine::PLOCAL_INC(int64_t op,SQObjectPtr &target, SQObjectPtr &a, SQObjectPtr &incr)
{
	SQObjectPtr trg;
	_RET_ON_FAIL(ARITH_OP( op , trg, a, incr));
	target = a;
	a = trg;
	return true;
}

bool rabbit::VirtualMachine::derefInc(int64_t op,SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix,int64_t selfidx)
{
	SQObjectPtr tmp, tself = self, tkey = key;
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

SQRESULT rabbit::VirtualMachine::Suspend()
{
	if (_suspended)
		return sq_throwerror(this, _SC("cannot suspend an already suspended vm"));
	if (_nnativecalls!=2)
		return sq_throwerror(this, _SC("cannot suspend through native calls/metamethods"));
	return SQ_SUSPEND_FLAG;
}


#define _FINISH(howmuchtojump) {jump = howmuchtojump; return true; }
bool rabbit::VirtualMachine::FOREACH_OP(SQObjectPtr &o1,SQObjectPtr &o2,SQObjectPtr
&o3,SQObjectPtr &o4,int64_t SQ_UNUSED_ARG(arg_2),int exitpos,int &jump)
{
	int64_t nrefidx;
	switch(sq_type(o1)) {
	case OT_TABLE:
		if((nrefidx = _table(o1)->next(false,o4, o2, o3)) == -1) _FINISH(exitpos);
		o4 = (int64_t)nrefidx; _FINISH(1);
	case OT_ARRAY:
		if((nrefidx = _array(o1)->next(o4, o2, o3)) == -1) _FINISH(exitpos);
		o4 = (int64_t) nrefidx; _FINISH(1);
	case OT_STRING:
		if((nrefidx = _string(o1)->next(o4, o2, o3)) == -1)_FINISH(exitpos);
		o4 = (int64_t)nrefidx; _FINISH(1);
	case OT_CLASS:
		if((nrefidx = _class(o1)->next(o4, o2, o3)) == -1)_FINISH(exitpos);
		o4 = (int64_t)nrefidx; _FINISH(1);
	case OT_USERDATA:
	case OT_INSTANCE:
		if(_delegable(o1)->_delegate) {
			SQObjectPtr itr;
			SQObjectPtr closure;
			if(_delegable(o1)->getMetaMethod(this, MT_NEXTI, closure)) {
				push(o1);
				push(o4);
				if(callMetaMethod(closure, MT_NEXTI, 2, itr)) {
					o4 = o2 = itr;
					if(sq_type(itr) == OT_NULL) _FINISH(exitpos);
					if(!get(o1, itr, o3, 0, DONT_FALL_BACK)) {
						raise_error(_SC("_nexti returned an invalid idx")); // cloud be changed
						return false;
					}
					_FINISH(1);
				}
				else {
					return false;
				}
			}
			raise_error(_SC("_nexti failed"));
			return false;
		}
		break;
	case OT_GENERATOR:
		if(_generator(o1)->_state == SQGenerator::eDead) _FINISH(exitpos);
		if(_generator(o1)->_state == SQGenerator::eSuspended) {
			int64_t idx = 0;
			if(sq_type(o4) == OT_INTEGER) {
				idx = _integer(o4) + 1;
			}
			o2 = idx;
			o4 = idx;
			_generator(o1)->resume(this, o3);
			_FINISH(0);
		}
	default:
		raise_error(_SC("cannot iterate %s"), getTypeName(o1));
	}
	return false; //cannot be hit(just to avoid warnings)
}

#define COND_LITERAL (arg3!=0?ci->_literals[arg1]:STK(arg1))

#define SQ_THROW() { goto exception_trap; }

#define _GUARD(exp) { if(!exp) { SQ_THROW();} }

bool rabbit::VirtualMachine::CLOSURE_OP(SQObjectPtr &target, SQFunctionProto *func)
{
	int64_t nouters;
	SQClosure *closure = SQClosure::create(_ss(this), func,_table(_roottable)->getWeakRef(OT_TABLE));
	if((nouters = func->_noutervalues)) {
		for(int64_t i = 0; i<nouters; i++) {
			SQOuterVar &v = func->_outervalues[i];
			switch(v._type){
			case otLOCAL:
				findOuter(closure->_outervalues[i], &STK(_integer(v._src)));
				break;
			case otOUTER:
				closure->_outervalues[i] = _closure(ci->_closure)->_outervalues[_integer(v._src)];
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


bool rabbit::VirtualMachine::CLASS_OP(SQObjectPtr &target,int64_t baseclass,int64_t attributes)
{
	SQClass *base = NULL;
	SQObjectPtr attrs;
	if(baseclass != -1) {
		if(sq_type(_stack[_stackbase+baseclass]) != OT_CLASS) { raise_error(_SC("trying to inherit from a %s"),getTypeName(_stack[_stackbase+baseclass])); return false; }
		base = _class(_stack[_stackbase + baseclass]);
	}
	if(attributes != MAX_FUNC_STACKSIZE) {
		attrs = _stack[_stackbase+attributes];
	}
	target = SQClass::create(_ss(this),base);
	if(sq_type(_class(target)->_metamethods[MT_INHERITED]) != OT_NULL) {
		int nparams = 2;
		SQObjectPtr ret;
		push(target); push(attrs);
		if(!call(_class(target)->_metamethods[MT_INHERITED],nparams,_top - nparams, ret, false)) {
			pop(nparams);
			return false;
		}
		pop(nparams);
	}
	_class(target)->_attributes = attrs;
	return true;
}

bool rabbit::VirtualMachine::isEqual(const SQObjectPtr &o1,const SQObjectPtr &o2,bool &res)
{
	if(sq_type(o1) == sq_type(o2)) {
		res = (_rawval(o1) == _rawval(o2));
	}
	else {
		if(sq_isnumeric(o1) && sq_isnumeric(o2)) {
			res = (tofloat(o1) == tofloat(o2));
		}
		else {
			res = false;
		}
	}
	return true;
}

bool rabbit::VirtualMachine::IsFalse(SQObjectPtr &o)
{
	if(((sq_type(o) & SQOBJECT_CANBEFALSE)
		&& ( ((sq_type(o) == OT_FLOAT) && (_float(o) == float_t(0.0))) ))
#if !defined(SQUSEDOUBLE) || (defined(SQUSEDOUBLE) && defined(_SQ64))
		|| (_integer(o) == 0) )  //OT_NULL|OT_INTEGER|OT_BOOL
#else
		|| (((type(o) != OT_FLOAT) && (_integer(o) == 0))) )  //OT_NULL|OT_INTEGER|OT_BOOL
#endif
	{
		return true;
	}
	return false;
}
extern SQInstructionDesc g_InstrDesc[];
bool rabbit::VirtualMachine::execute(SQObjectPtr &closure, int64_t nargs, int64_t stackbase,SQObjectPtr &outres, SQBool raiseerror,ExecutionType et)
{
	if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) { raise_error(_SC("Native stack overflow")); return false; }
	_nnativecalls++;
	AutoDec ad(&_nnativecalls);
	int64_t traps = 0;
	callInfo *prevci = ci;

	switch(et) {
		case ET_CALL: {
			temp_reg = closure;
			if(!startcall(_closure(temp_reg), _top - nargs, nargs, stackbase, false)) {
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
		case ET_RESUME_GENERATOR: _generator(closure)->resume(this, outres); ci->_root = SQTrue; traps += ci->_etraps; break;
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
			const SQInstruction &_i_ = *ci->_ip++;
			//dumpstack(_stackbase);
			//scprintf("\n[%d] %s %d %d %d %d\n",ci->_ip-_closure(ci->_closure)->_function->_instructions,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
			switch(_i_.op)
			{
			case _OP_LINE: if (_debughook) callDebugHook(_SC('l'),arg1); continue;
			case _OP_LOAD: TARGET = ci->_literals[arg1]; continue;
			case _OP_LOADINT:
#ifndef _SQ64
				TARGET = (int64_t)arg1; continue;
#else
				TARGET = (int64_t)((int32_t)arg1); continue;
#endif
			case _OP_LOADFLOAT: TARGET = *((const float_t *)&arg1); continue;
			case _OP_DLOAD: TARGET = ci->_literals[arg1]; STK(arg2) = ci->_literals[arg3];continue;
			case _OP_TAILCALL:{
				SQObjectPtr &t = STK(arg1);
				if (sq_type(t) == OT_CLOSURE
					&& (!_closure(t)->_function->_bgenerator)){
					SQObjectPtr clo = t;
					int64_t last_top = _top;
					if(_openouters) closeOuters(&(_stack[_stackbase]));
					for (int64_t i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
					_GUARD(startcall(_closure(clo), ci->_target, arg3, _stackbase, true));
					if (last_top >= _top) {
						_top = last_top;
					}
					continue;
				}
							  }
			case _OP_CALL: {
					SQObjectPtr clo = STK(arg1);
					switch (sq_type(clo)) {
					case OT_CLOSURE:
						_GUARD(startcall(_closure(clo), sarg0, arg3, _stackbase+arg2, false));
						continue;
					case OT_NATIVECLOSURE: {
						bool suspend;
						bool tailcall;
						_GUARD(callNative(_nativeclosure(clo), arg3, _stackbase+arg2, clo, (int32_t)sarg0, suspend, tailcall));
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
					case OT_CLASS:{
						SQObjectPtr inst;
						_GUARD(createClassInstance(_class(clo),inst,clo));
						if(sarg0 != -1) {
							STK(arg0) = inst;
						}
						int64_t stkbase;
						switch(sq_type(clo)) {
							case OT_CLOSURE:
								stkbase = _stackbase+arg2;
								_stack[stkbase] = inst;
								_GUARD(startcall(_closure(clo), -1, arg3, stkbase, false));
								break;
							case OT_NATIVECLOSURE:
								bool dummy;
								stkbase = _stackbase+arg2;
								_stack[stkbase] = inst;
								_GUARD(callNative(_nativeclosure(clo), arg3, stkbase, clo, -1, dummy, dummy));
								break;
							default: break; //shutup GCC 4.x
						}
						}
						break;
					case OT_TABLE:
					case OT_USERDATA:
					case OT_INSTANCE:{
						SQObjectPtr closure;
						if(_delegable(clo)->_delegate && _delegable(clo)->getMetaMethod(this,MT_CALL,closure)) {
							push(clo);
							for (int64_t i = 0; i < arg3; i++) push(STK(arg2 + i));
							if(!callMetaMethod(closure, MT_CALL, arg3+1, clo)) SQ_THROW();
							if(sarg0 != -1) {
								STK(arg0) = clo;
							}
							break;
						}

						//raise_error(_SC("attempt to call '%s'"), getTypeName(clo));
						//SQ_THROW();
					  }
					default:
						raise_error(_SC("attempt to call '%s'"), getTypeName(clo));
						SQ_THROW();
					}
				}
				  continue;
			case _OP_PREPCALL:
			case _OP_PREPCALLK: {
					SQObjectPtr &key = _i_.op == _OP_PREPCALLK?(ci->_literals)[arg1]:STK(arg1);
					SQObjectPtr &o = STK(arg2);
					if (!get(o, key, temp_reg,0,arg2)) {
						SQ_THROW();
					}
					STK(arg3) = o;
					_Swap(TARGET,temp_reg);//TARGET = temp_reg;
				}
				continue;
			case _OP_GETK:
				if (!get(STK(arg2), ci->_literals[arg1], temp_reg, 0,arg2)) { SQ_THROW();}
				_Swap(TARGET,temp_reg);//TARGET = temp_reg;
				continue;
			case _OP_MOVE: TARGET = STK(arg1); continue;
			case _OP_NEWSLOT:
				_GUARD(newSlot(STK(arg1), STK(arg2), STK(arg3),false));
				if(arg0 != 0xFF) TARGET = STK(arg3);
				continue;
			case _OP_DELETE: _GUARD(deleteSlot(STK(arg1), STK(arg2), TARGET)); continue;
			case _OP_SET:
				if (!set(STK(arg1), STK(arg2), STK(arg3),arg1)) { SQ_THROW(); }
				if (arg0 != 0xFF) TARGET = STK(arg3);
				continue;
			case _OP_GET:
				if (!get(STK(arg1), STK(arg2), temp_reg, 0,arg1)) { SQ_THROW(); }
				_Swap(TARGET,temp_reg);//TARGET = temp_reg;
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
			case _OP_DIV: _ARITH_NOZERO(/,TARGET,STK(arg2),STK(arg1),_SC("division by zero")); continue;
			case _OP_MOD: ARITH_OP('%',TARGET,STK(arg2),STK(arg1)); continue;
			case _OP_BITW:  _GUARD(BW_OP( arg3,TARGET,STK(arg2),STK(arg1))); continue;
			case _OP_RETURN:
				if((ci)->_generator) {
					(ci)->_generator->kill();
				}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps==0);
					//outres = temp_reg;
					_Swap(outres,temp_reg);
					return true;
				}
				continue;
			case _OP_LOADNULLS:{ for(int32_t n=0; n < arg1; n++) STK(arg0+n).Null(); }continue;
			case _OP_LOADROOT:  {
				rabbit::WeakRef *w = _closure(ci->_closure)->_root;
				if(sq_type(w->_obj) != OT_NULL) {
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
				SQClosure *cur_cls = _closure(ci->_closure);
				SQOuter *otr = _outer(cur_cls->_outervalues[arg1]);
				TARGET = *(otr->_valptr);
				}
			continue;
			case _OP_SETOUTER: {
				SQClosure *cur_cls = _closure(ci->_closure);
				SQOuter   *otr = _outer(cur_cls->_outervalues[arg1]);
				*(otr->_valptr) = STK(arg2);
				if(arg0 != 0xFF) {
					TARGET = STK(arg2);
				}
				}
			continue;
			case _OP_NEWOBJ:
				switch(arg3) {
					case NOT_TABLE: TARGET = SQTable::create(_ss(this), arg1); continue;
					case NOT_ARRAY: TARGET = rabbit::Array::create(_ss(this), 0); _array(TARGET)->reserve(arg1); continue;
					case NOT_CLASS: _GUARD(CLASS_OP(TARGET,arg1,arg2)); continue;
					default: assert(0); continue;
				}
			case _OP_APPENDARRAY:
				{
					SQObject val;
					val._unVal.raw = 0;
				switch(arg2) {
				case AAT_STACK:
					val = STK(arg1); break;
				case AAT_LITERAL:
					val = ci->_literals[arg1]; break;
				case AAT_INT:
					val._type = OT_INTEGER;
#ifndef _SQ64
					val._unVal.nInteger = (int64_t)arg1;
#else
					val._unVal.nInteger = (int64_t)((int32_t)arg1);
#endif
					break;
				case AAT_FLOAT:
					val._type = OT_FLOAT;
					val._unVal.fFloat = *((const float_t *)&arg1);
					break;
				case AAT_BOOL:
					val._type = OT_BOOL;
					val._unVal.nInteger = arg1;
					break;
				default: val._type = OT_INTEGER; assert(0); break;

				}
				_array(STK(arg0))->append(val); continue;
				}
			case _OP_COMPARITH: {
				int64_t selfidx = (((uint64_t)arg1&0xFFFF0000)>>16);
				_GUARD(derefInc(arg3, TARGET, STK(selfidx), STK(arg2), STK(arg1&0x0000FFFF), false, selfidx));
								}
				continue;
			case _OP_INC: {SQObjectPtr o(sarg3); _GUARD(derefInc('+',TARGET, STK(arg1), STK(arg2), o, false, arg1));} continue;
			case _OP_INCL: {
				SQObjectPtr &a = STK(arg1);
				if(sq_type(a) == OT_INTEGER) {
					a._unVal.nInteger = _integer(a) + sarg3;
				}
				else {
					SQObjectPtr o(sarg3); //_GUARD(LOCAL_INC('+',TARGET, STK(arg1), o));
					_ARITH_(+,a,a,o);
				}
						   } continue;
			case _OP_PINC: {SQObjectPtr o(sarg3); _GUARD(derefInc('+',TARGET, STK(arg1), STK(arg2), o, true, arg1));} continue;
			case _OP_PINCL: {
				SQObjectPtr &a = STK(arg1);
				if(sq_type(a) == OT_INTEGER) {
					TARGET = a;
					a._unVal.nInteger = _integer(a) + sarg3;
				}
				else {
					SQObjectPtr o(sarg3); _GUARD(PLOCAL_INC('+',TARGET, STK(arg1), o));
				}

						} continue;
			case _OP_CMP:   _GUARD(CMP_OP((CmpOP)arg3,STK(arg2),STK(arg1),TARGET))  continue;
			case _OP_EXISTS: TARGET = get(STK(arg1), STK(arg2), temp_reg, GET_FLAG_DO_NOT_RAISE_ERROR | GET_FLAG_RAW, DONT_FALL_BACK) ? true : false; continue;
			case _OP_INSTANCEOF:
				if(sq_type(STK(arg1)) != OT_CLASS)
				{raise_error(_SC("cannot apply instanceof between a %s and a %s"),getTypeName(STK(arg1)),getTypeName(STK(arg2))); SQ_THROW();}
				TARGET = (sq_type(STK(arg2)) == OT_INSTANCE) ? (_instance(STK(arg2))->instanceOf(_class(STK(arg1)))?true:false) : false;
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
				if(sq_type(STK(arg1)) == OT_INTEGER) {
					int64_t t = _integer(STK(arg1));
					TARGET = int64_t(~t);
					continue;
				}
				raise_error(_SC("attempt to perform a bitwise op on a %s"), getTypeName(STK(arg1)));
				SQ_THROW();
			case _OP_CLOSURE: {
				SQClosure *c = ci->_closure._unVal.pClosure;
				SQFunctionProto *fp = c->_function;
				if(!CLOSURE_OP(TARGET,fp->_functions[arg1]._unVal.pFunctionProto)) { SQ_THROW(); }
				continue;
			}
			case _OP_YIELD:{
				if(ci->_generator) {
					if(sarg1 != MAX_FUNC_STACKSIZE) temp_reg = STK(arg1);
					_GUARD(ci->_generator->yield(this,arg2));
					traps -= ci->_etraps;
					if(sarg1 != MAX_FUNC_STACKSIZE) _Swap(STK(arg1),temp_reg);//STK(arg1) = temp_reg;
				}
				else { raise_error(_SC("trying to yield a '%s',only genenerator can be yielded"), getTypeName(ci->_generator)); SQ_THROW();}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps == 0);
					outres = temp_reg;
					return true;
				}

				}
				continue;
			case _OP_RESUME:
				if(sq_type(STK(arg1)) != OT_GENERATOR){ raise_error(_SC("trying to resume a '%s',only genenerator can be resumed"), getTypeName(STK(arg1))); SQ_THROW();}
				_GUARD(_generator(STK(arg1))->resume(this, TARGET));
				traps += ci->_etraps;
				continue;
			case _OP_FOREACH:{ int tojump;
				_GUARD(FOREACH_OP(STK(arg0),STK(arg2),STK(arg2+1),STK(arg2+2),arg2,sarg1,tojump));
				ci->_ip += tojump; }
				continue;
			case _OP_POSTFOREACH:
				assert(sq_type(STK(arg0)) == OT_GENERATOR);
				if(_generator(STK(arg0))->_state == SQGenerator::eDead)
					ci->_ip += (sarg1 - 1);
				continue;
			case _OP_CLONE: _GUARD(clone(STK(arg1), TARGET)); continue;
			case _OP_TYPEOF: _GUARD(typeOf(STK(arg1), TARGET)) continue;
			case _OP_PUSHTRAP:{
				SQInstruction *_iv = _closure(ci->_closure)->_function->_instructions;
				_etraps.pushBack(SQExceptionTrap(_top,_stackbase, &_iv[(ci->_ip-_iv)+arg1], arg0)); traps++;
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
				_GUARD(newSlotA(STK(arg1),STK(arg2),STK(arg3),(arg0&NEW_SLOT_ATTRIBUTES_FLAG) ? STK(arg2-1) : SQObjectPtr(),(arg0&NEW_SLOT_STATIC_FLAG)?true:false,false));
				continue;
			case _OP_GETBASE:{
				SQClosure *clo = _closure(ci->_closure);
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
		SQObjectPtr currerror = _lasterror;
//	  dumpstack(_stackbase);
//	  int64_t n = 0;
		int64_t last_top = _top;

		if(_ss(this)->_notifyallexceptions || (!traps && raiseerror)) callerrorHandler(currerror);

		while( ci ) {
			if(ci->_etraps > 0) {
				SQExceptionTrap &et = _etraps.back();
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
						callDebugHook(_SC('r'));
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

bool rabbit::VirtualMachine::createClassInstance(SQClass *theclass, SQObjectPtr &inst, SQObjectPtr &constructor)
{
	inst = theclass->createInstance();
	if(!theclass->getConstructor(constructor)) {
		constructor.Null();
	}
	return true;
}

void rabbit::VirtualMachine::callerrorHandler(SQObjectPtr &error)
{
	if(sq_type(_errorhandler) != OT_NULL) {
		SQObjectPtr out;
		push(_roottable); push(error);
		call(_errorhandler, 2, _top-2, out,SQFalse);
		pop(2);
	}
}


void rabbit::VirtualMachine::callDebugHook(int64_t type,int64_t forcedline)
{
	_debughook = false;
	SQFunctionProto *func=_closure(ci->_closure)->_function;
	if(_debughook_native) {
		const SQChar *src = sq_type(func->_sourcename) == OT_STRING?_stringval(func->_sourcename):NULL;
		const SQChar *fname = sq_type(func->_name) == OT_STRING?_stringval(func->_name):NULL;
		int64_t line = forcedline?forcedline:func->getLine(ci->_ip);
		_debughook_native(this,type,src,line,fname);
	}
	else {
		SQObjectPtr temp_reg;
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

bool rabbit::VirtualMachine::callNative(SQNativeClosure *nclosure, int64_t nargs, int64_t newbase, SQObjectPtr &retval, int32_t target,bool &suspend, bool &tailcall)
{
	int64_t nparamscheck = nclosure->_nparamscheck;
	int64_t newtop = newbase + nargs + nclosure->_noutervalues;

	if (_nnativecalls + 1 > MAX_NATIVE_CALLS) {
		raise_error(_SC("Native stack overflow"));
		return false;
	}

	if(nparamscheck && (((nparamscheck > 0) && (nparamscheck != nargs)) ||
		((nparamscheck < 0) && (nargs < (-nparamscheck)))))
	{
		raise_error(_SC("wrong number of parameters"));
		return false;
	}

	int64_t tcs;
	SQIntVec &tc = nclosure->_typecheck;
	if((tcs = tc.size())) {
		for(int64_t i = 0; i < nargs && i < tcs; i++) {
			if((tc[i] != -1) && !(sq_type(_stack[newbase+i]) & tc[i])) {
				raise_ParamTypeerror(i,tc[i], sq_type(_stack[newbase+i]));
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

bool rabbit::VirtualMachine::tailcall(SQClosure *closure, int64_t parambase,int64_t nparams)
{
	int64_t last_top = _top;
	SQObjectPtr clo = closure;
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

#define FALLBACK_OK		 0
#define FALLBACK_NO_MATCH   1
#define FALLBACK_ERROR	  2

bool rabbit::VirtualMachine::get(const SQObjectPtr &self, const SQObjectPtr &key, SQObjectPtr &dest, uint64_t getflags, int64_t selfidx)
{
	switch(sq_type(self)){
	case OT_TABLE:
		if(_table(self)->get(key,dest))return true;
		break;
	case OT_ARRAY:
		if (sq_isnumeric(key)) { if (_array(self)->get(tointeger(key), dest)) { return true; } if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) raise_Idxerror(key); return false; }
		break;
	case OT_INSTANCE:
		if(_instance(self)->get(key,dest)) return true;
		break;
	case OT_CLASS:
		if(_class(self)->get(key,dest)) return true;
		break;
	case OT_STRING:
		if(sq_isnumeric(key)){
			int64_t n = tointeger(key);
			int64_t len = _string(self)->_len;
			if (n < 0) { n += len; }
			if (n >= 0 && n < len) {
				dest = int64_t(_stringval(self)[n]);
				return true;
			}
			if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) raise_Idxerror(key);
			return false;
		}
		break;
	default:break; //shut up compiler
	}
	if ((getflags & GET_FLAG_RAW) == 0) {
		switch(fallBackGet(self,key,dest)) {
			case FALLBACK_OK: return true; //okie
			case FALLBACK_NO_MATCH: break; //keep falling back
			case FALLBACK_ERROR: return false; // the metamethod failed
		}
		if(invokeDefaultDelegate(self,key,dest)) {
			return true;
		}
	}
//#ifdef ROOT_FALLBACK
	if(selfidx == 0) {
		rabbit::WeakRef *w = _closure(ci->_closure)->_root;
		if(sq_type(w->_obj) != OT_NULL)
		{
			if(get(*((const SQObjectPtr *)&w->_obj),key,dest,0,DONT_FALL_BACK)) return true;
		}

	}
//#endif
	if ((getflags & GET_FLAG_DO_NOT_RAISE_ERROR) == 0) raise_Idxerror(key);
	return false;
}

bool rabbit::VirtualMachine::invokeDefaultDelegate(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest)
{
	SQTable *ddel = NULL;
	switch(sq_type(self)) {
		case OT_CLASS: ddel = _class_ddel; break;
		case OT_TABLE: ddel = _table_ddel; break;
		case OT_ARRAY: ddel = _array_ddel; break;
		case OT_STRING: ddel = _string_ddel; break;
		case OT_INSTANCE: ddel = _instance_ddel; break;
		case OT_INTEGER:case OT_FLOAT:case OT_BOOL: ddel = _number_ddel; break;
		case OT_GENERATOR: ddel = _generator_ddel; break;
		case OT_CLOSURE: case OT_NATIVECLOSURE: ddel = _closure_ddel; break;
		case OT_THREAD: ddel = _thread_ddel; break;
		case OT_WEAKREF: ddel = _weakref_ddel; break;
		default: return false;
	}
	return  ddel->get(key,dest);
}


int64_t rabbit::VirtualMachine::fallBackGet(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest)
{
	switch(sq_type(self)){
	case OT_TABLE:
	case OT_USERDATA:
		//delegation
		if(_delegable(self)->_delegate) {
			if(get(SQObjectPtr(_delegable(self)->_delegate),key,dest,0,DONT_FALL_BACK)) return FALLBACK_OK;
		}
		else {
			return FALLBACK_NO_MATCH;
		}
		//go through
	case OT_INSTANCE: {
		SQObjectPtr closure;
		if(_delegable(self)->getMetaMethod(this, MT_GET, closure)) {
			push(self);push(key);
			_nmetamethodscall++;
			AutoDec ad(&_nmetamethodscall);
			if(call(closure, 2, _top - 2, dest, SQFalse)) {
				pop(2);
				return FALLBACK_OK;
			}
			else {
				pop(2);
				if(sq_type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
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

bool rabbit::VirtualMachine::set(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,int64_t selfidx)
{
	switch(sq_type(self)){
	case OT_TABLE:
		if(_table(self)->set(key,val)) return true;
		break;
	case OT_INSTANCE:
		if(_instance(self)->set(key,val)) return true;
		break;
	case OT_ARRAY:
		if(!sq_isnumeric(key)) { raise_error(_SC("indexing %s with %s"),getTypeName(self),getTypeName(key)); return false; }
		if(!_array(self)->set(tointeger(key),val)) {
			raise_Idxerror(key);
			return false;
		}
		return true;
	case OT_USERDATA: break; // must fall back
	default:
		raise_error(_SC("trying to set '%s'"),getTypeName(self));
		return false;
	}

	switch(fallBackSet(self,key,val)) {
		case FALLBACK_OK: return true; //okie
		case FALLBACK_NO_MATCH: break; //keep falling back
		case FALLBACK_ERROR: return false; // the metamethod failed
	}
	if(selfidx == 0) {
		if(_table(_roottable)->set(key,val))
			return true;
	}
	raise_Idxerror(key);
	return false;
}

int64_t rabbit::VirtualMachine::fallBackSet(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val)
{
	switch(sq_type(self)) {
	case OT_TABLE:
		if(_table(self)->_delegate) {
			if(set(_table(self)->_delegate,key,val,DONT_FALL_BACK)) return FALLBACK_OK;
		}
		//keps on going
	case OT_INSTANCE:
	case OT_USERDATA:{
		SQObjectPtr closure;
		SQObjectPtr t;
		if(_delegable(self)->getMetaMethod(this, MT_SET, closure)) {
			push(self);push(key);push(val);
			_nmetamethodscall++;
			AutoDec ad(&_nmetamethodscall);
			if(call(closure, 3, _top - 3, t, SQFalse)) {
				pop(3);
				return FALLBACK_OK;
			}
			else {
				pop(3);
				if(sq_type(_lasterror) != OT_NULL) { //NULL means "clean failure" (not found)
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

bool rabbit::VirtualMachine::clone(const SQObjectPtr &self,SQObjectPtr &target)
{
	SQObjectPtr temp_reg;
	SQObjectPtr newobj;
	switch(sq_type(self)){
	case OT_TABLE:
		newobj = _table(self)->clone();
		goto cloned_mt;
	case OT_INSTANCE: {
		newobj = _instance(self)->clone(_ss(this));
cloned_mt:
		SQObjectPtr closure;
		if(_delegable(newobj)->_delegate && _delegable(newobj)->getMetaMethod(this,MT_CLONED,closure)) {
			push(newobj);
			push(self);
			if(!callMetaMethod(closure,MT_CLONED,2,temp_reg))
				return false;
		}
		}
		target = newobj;
		return true;
	case OT_ARRAY:
		target = _array(self)->clone();
		return true;
	default:
		raise_error(_SC("cloning a %s"), getTypeName(self));
		return false;
	}
}

bool rabbit::VirtualMachine::newSlotA(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,const SQObjectPtr &attrs,bool bstatic,bool raw)
{
	if(sq_type(self) != OT_CLASS) {
		raise_error(_SC("object must be a class"));
		return false;
	}
	SQClass *c = _class(self);
	if(!raw) {
		SQObjectPtr &mm = c->_metamethods[MT_NEWMEMBER];
		if(sq_type(mm) != OT_NULL ) {
			push(self); push(key); push(val);
			push(attrs);
			push(bstatic);
			return callMetaMethod(mm,MT_NEWMEMBER,5,temp_reg);
		}
	}
	if(!newSlot(self, key, val,bstatic))
		return false;
	if(sq_type(attrs) != OT_NULL) {
		c->setAttributes(key,attrs);
	}
	return true;
}

bool rabbit::VirtualMachine::newSlot(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val,bool bstatic)
{
	if(sq_type(key) == OT_NULL) { raise_error(_SC("null cannot be used as index")); return false; }
	switch(sq_type(self)) {
	case OT_TABLE: {
		bool rawcall = true;
		if(_table(self)->_delegate) {
			SQObjectPtr res;
			if(!_table(self)->get(key,res)) {
				SQObjectPtr closure;
				if(_delegable(self)->_delegate && _delegable(self)->getMetaMethod(this,MT_NEWSLOT,closure)) {
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
		if(rawcall) _table(self)->newSlot(key,val); //cannot fail

		break;}
	case OT_INSTANCE: {
		SQObjectPtr res;
		SQObjectPtr closure;
		if(_delegable(self)->_delegate && _delegable(self)->getMetaMethod(this,MT_NEWSLOT,closure)) {
			push(self);push(key);push(val);
			if(!callMetaMethod(closure,MT_NEWSLOT,3,res)) {
				return false;
			}
			break;
		}
		raise_error(_SC("class instances do not support the new slot operator"));
		return false;
		break;}
	case OT_CLASS:
		if(!_class(self)->newSlot(_ss(this),key,val,bstatic)) {
			if(_class(self)->_locked) {
				raise_error(_SC("trying to modify a class that has already been instantiated"));
				return false;
			}
			else {
				SQObjectPtr oval = printObjVal(key);
				raise_error(_SC("the property '%s' already exists"),_stringval(oval));
				return false;
			}
		}
		break;
	default:
		raise_error(_SC("indexing %s with %s"),getTypeName(self),getTypeName(key));
		return false;
		break;
	}
	return true;
}



bool rabbit::VirtualMachine::deleteSlot(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &res)
{
	switch(sq_type(self)) {
	case OT_TABLE:
	case OT_INSTANCE:
	case OT_USERDATA: {
		SQObjectPtr t;
		//bool handled = false;
		SQObjectPtr closure;
		if(_delegable(self)->_delegate && _delegable(self)->getMetaMethod(this,MT_DELSLOT,closure)) {
			push(self);push(key);
			return callMetaMethod(closure,MT_DELSLOT,2,res);
		}
		else {
			if(sq_type(self) == OT_TABLE) {
				if(_table(self)->get(key,t)) {
					_table(self)->remove(key);
				}
				else {
					raise_Idxerror((const SQObject &)key);
					return false;
				}
			}
			else {
				raise_error(_SC("cannot delete a slot from %s"),getTypeName(self));
				return false;
			}
		}
		res = t;
				}
		break;
	default:
		raise_error(_SC("attempt to delete a slot from a %s"),getTypeName(self));
		return false;
	}
	return true;
}

bool rabbit::VirtualMachine::call(SQObjectPtr &closure,int64_t nparams,int64_t stackbase,SQObjectPtr &outres,SQBool raiseerror)
{
#ifdef _DEBUG
int64_t prevstackbase = _stackbase;
#endif
	switch(sq_type(closure)) {
	case OT_CLOSURE:
		return execute(closure, nparams, stackbase, outres, raiseerror);
		break;
	case OT_NATIVECLOSURE:{
		bool dummy;
		return callNative(_nativeclosure(closure), nparams, stackbase, outres, -1, dummy, dummy);

						  }
		break;
	case OT_CLASS: {
		SQObjectPtr constr;
		SQObjectPtr temp;
		createClassInstance(_class(closure),outres,constr);
		SQObjectType ctype = sq_type(constr);
		if (ctype == OT_NATIVECLOSURE || ctype == OT_CLOSURE) {
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

bool rabbit::VirtualMachine::callMetaMethod(SQObjectPtr &closure,SQMetaMethod SQ_UNUSED_ARG(mm),int64_t nparams,SQObjectPtr &outres)
{
	//SQObjectPtr closure;

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

void rabbit::VirtualMachine::findOuter(SQObjectPtr &target, SQObjectPtr *stackindex)
{
	SQOuter **pp = &_openouters;
	SQOuter *p;
	SQOuter *otr;

	while ((p = *pp) != NULL && p->_valptr >= stackindex) {
		if (p->_valptr == stackindex) {
			target = SQObjectPtr(p);
			return;
		}
		pp = &p->_next;
	}
	otr = SQOuter::create(_ss(this), stackindex);
	otr->_next = *pp;
	// TODO: rework this, this is absolutly not safe...
	otr->_idx  = (stackindex - &_stack[0]);
	__ObjaddRef(otr);
	*pp = otr;
	target = SQObjectPtr(otr);
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
			raise_error(_SC("stack overflow, cannot resize stack while in a metamethod"));
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
	SQOuter *p = _openouters;
	while (p) {
		p->_valptr = &_stack[p->_idx];
		p = p->_next;
	}
}

void rabbit::VirtualMachine::closeOuters(SQObjectPtr *stackindex) {
  SQOuter *p;
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
void rabbit::VirtualMachine::push(const SQObjectPtr &o) { _stack[_top++] = o; }
SQObjectPtr &rabbit::VirtualMachine::top() { return _stack[_top-1]; }
SQObjectPtr &rabbit::VirtualMachine::popGet() { return _stack[--_top]; }
SQObjectPtr &rabbit::VirtualMachine::getUp(int64_t n) { return _stack[_top+n]; }
SQObjectPtr &rabbit::VirtualMachine::getAt(int64_t n) { return _stack[n]; }

#ifdef _DEBUG_DUMP
void rabbit::VirtualMachine::dumpstack(int64_t stackbase,bool dumpall)
{
	int64_t size=dumpall?_stack.size():_top;
	int64_t n=0;
	scprintf(_SC("\n>>>>stack dump<<<<\n"));
	callInfo &ci=_callsstack[_callsstacksize-1];
	scprintf(_SC("IP: %p\n"),ci._ip);
	scprintf(_SC("prev stack base: %d\n"),ci._prevstkbase);
	scprintf(_SC("prev top: %d\n"),ci._prevtop);
	for(int64_t i=0;i<size;i++){
		SQObjectPtr &obj=_stack[i];
		if(stackbase==i)scprintf(_SC(">"));else scprintf(_SC(" "));
		scprintf(_SC("[" _PRINT_INT_FMT "]:"),n);
		switch(sq_type(obj)){
		case OT_FLOAT:		  scprintf(_SC("FLOAT %.3f"),_float(obj));break;
		case OT_INTEGER:		scprintf(_SC("INTEGER " _PRINT_INT_FMT),_integer(obj));break;
		case OT_BOOL:		   scprintf(_SC("BOOL %s"),_integer(obj)?"true":"false");break;
		case OT_STRING:		 scprintf(_SC("STRING %s"),_stringval(obj));break;
		case OT_NULL:		   scprintf(_SC("NULL"));  break;
		case OT_TABLE:		  scprintf(_SC("TABLE %p[%p]"),_table(obj),_table(obj)->_delegate);break;
		case OT_ARRAY:		  scprintf(_SC("ARRAY %p"),_array(obj));break;
		case OT_CLOSURE:		scprintf(_SC("CLOSURE [%p]"),_closure(obj));break;
		case OT_NATIVECLOSURE:  scprintf(_SC("NATIVECLOSURE"));break;
		case OT_USERDATA:	   scprintf(_SC("USERDATA %p[%p]"),_userdataval(obj),_userdata(obj)->_delegate);break;
		case OT_GENERATOR:	  scprintf(_SC("GENERATOR %p"),_generator(obj));break;
		case OT_THREAD:		 scprintf(_SC("THREAD [%p]"),_thread(obj));break;
		case OT_USERPOINTER:	scprintf(_SC("USERPOINTER %p"),_userpointer(obj));break;
		case OT_CLASS:		  scprintf(_SC("CLASS %p"),_class(obj));break;
		case OT_INSTANCE:	   scprintf(_SC("INSTANCE %p"),_instance(obj));break;
		case OT_WEAKREF:		scprintf(_SC("WEAKERF %p"),_weakref(obj));break;
		default:
			assert(0);
			break;
		};
		scprintf(_SC("\n"));
		++n;
	}
}



#endif
