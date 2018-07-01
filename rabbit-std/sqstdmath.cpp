/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/rabbit.hpp>
#include <math.h>
#include <stdlib.h>
#include <rabbit-std/sqstdmath.hpp>

#define SINGLE_ARG_FUNC(_funcname) static int64_t math_##_funcname(rabbit::VirtualMachine* v){ \
	float_t f; \
	sq_getfloat(v,2,&f); \
	sq_pushfloat(v,(float_t)_funcname(f)); \
	return 1; \
}

#define TWO_ARGS_FUNC(_funcname) static int64_t math_##_funcname(rabbit::VirtualMachine* v){ \
	float_t p1,p2; \
	sq_getfloat(v,2,&p1); \
	sq_getfloat(v,3,&p2); \
	sq_pushfloat(v,(float_t)_funcname(p1,p2)); \
	return 1; \
}

static int64_t math_srand(rabbit::VirtualMachine* v)
{
	int64_t i;
	if(SQ_FAILED(sq_getinteger(v,2,&i)))
		return sq_throwerror(v,_SC("invalid param"));
	srand((unsigned int)i);
	return 0;
}

static int64_t math_rand(rabbit::VirtualMachine* v)
{
	sq_pushinteger(v,rand());
	return 1;
}

static int64_t math_abs(rabbit::VirtualMachine* v)
{
	int64_t n;
	sq_getinteger(v,2,&n);
	sq_pushinteger(v,(int64_t)abs((int)n));
	return 1;
}

SINGLE_ARG_FUNC(sqrt)
SINGLE_ARG_FUNC(fabs)
SINGLE_ARG_FUNC(sin)
SINGLE_ARG_FUNC(cos)
SINGLE_ARG_FUNC(asin)
SINGLE_ARG_FUNC(acos)
SINGLE_ARG_FUNC(log)
SINGLE_ARG_FUNC(log10)
SINGLE_ARG_FUNC(tan)
SINGLE_ARG_FUNC(atan)
TWO_ARGS_FUNC(atan2)
TWO_ARGS_FUNC(pow)
SINGLE_ARG_FUNC(floor)
SINGLE_ARG_FUNC(ceil)
SINGLE_ARG_FUNC(exp)

#define _DECL_FUNC(name,nparams,tycheck) {_SC(#name),math_##name,nparams,tycheck}
static const rabbit::RegFunction mathlib_funcs[] = {
	_DECL_FUNC(sqrt,2,_SC(".n")),
	_DECL_FUNC(sin,2,_SC(".n")),
	_DECL_FUNC(cos,2,_SC(".n")),
	_DECL_FUNC(asin,2,_SC(".n")),
	_DECL_FUNC(acos,2,_SC(".n")),
	_DECL_FUNC(log,2,_SC(".n")),
	_DECL_FUNC(log10,2,_SC(".n")),
	_DECL_FUNC(tan,2,_SC(".n")),
	_DECL_FUNC(atan,2,_SC(".n")),
	_DECL_FUNC(atan2,3,_SC(".nn")),
	_DECL_FUNC(pow,3,_SC(".nn")),
	_DECL_FUNC(floor,2,_SC(".n")),
	_DECL_FUNC(ceil,2,_SC(".n")),
	_DECL_FUNC(exp,2,_SC(".n")),
	_DECL_FUNC(srand,2,_SC(".n")),
	_DECL_FUNC(rand,1,NULL),
	_DECL_FUNC(fabs,2,_SC(".n")),
	_DECL_FUNC(abs,2,_SC(".n")),
	{NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

rabbit::Result rabbit::std::register_mathlib(rabbit::VirtualMachine* v)
{
	int64_t i=0;
	while(mathlib_funcs[i].name!=0) {
		sq_pushstring(v,mathlib_funcs[i].name,-1);
		sq_newclosure(v,mathlib_funcs[i].f,0);
		sq_setparamscheck(v,mathlib_funcs[i].nparamscheck,mathlib_funcs[i].typemask);
		sq_setnativeclosurename(v,-1,mathlib_funcs[i].name);
		sq_newslot(v,-3,SQFalse);
		i++;
	}
	sq_pushstring(v,_SC("RAND_MAX"),-1);
	sq_pushinteger(v,RAND_MAX);
	sq_newslot(v,-3,SQFalse);
	sq_pushstring(v,_SC("PI"),-1);
	sq_pushfloat(v,(float_t)M_PI);
	sq_newslot(v,-3,SQFalse);
	return SQ_OK;
}
