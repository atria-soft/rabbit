/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/rabbit.hpp>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <rabbit-std/sqstdsystem.hpp>

#include <rabbit/RegFunction.hpp>

static int64_t _system_getenv(rabbit::VirtualMachine* v)
{
	const char *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
		sq_pushstring(v,getenv(s),-1);
		return 1;
	}
	return 0;
}


static int64_t _system_system(rabbit::VirtualMachine* v)
{
	const char *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
		sq_pushinteger(v,system(s));
		return 1;
	}
	return sq_throwerror(v,_SC("wrong param"));
}


static int64_t _system_clock(rabbit::VirtualMachine* v)
{
	sq_pushfloat(v,((float_t)clock())/(float_t)CLOCKS_PER_SEC);
	return 1;
}

static int64_t _system_time(rabbit::VirtualMachine* v)
{
	int64_t t = (int64_t)time(NULL);
	sq_pushinteger(v,t);
	return 1;
}

static int64_t _system_remove(rabbit::VirtualMachine* v)
{
	const char *s;
	sq_getstring(v,2,&s);
	if(remove(s)==-1)
		return sq_throwerror(v,_SC("remove() failed"));
	return 0;
}

static int64_t _system_rename(rabbit::VirtualMachine* v)
{
	const char *oldn,*newn;
	sq_getstring(v,2,&oldn);
	sq_getstring(v,3,&newn);
	if(rename(oldn,newn)==-1)
		return sq_throwerror(v,_SC("rename() failed"));
	return 0;
}

static void _set_integer_slot(rabbit::VirtualMachine* v,const char *name,int64_t val)
{
	sq_pushstring(v,name,-1);
	sq_pushinteger(v,val);
	sq_rawset(v,-3);
}

static int64_t _system_date(rabbit::VirtualMachine* v)
{
	time_t t;
	int64_t it;
	int64_t format = 'l';
	if(sq_gettop(v) > 1) {
		sq_getinteger(v,2,&it);
		t = it;
		if(sq_gettop(v) > 2) {
			sq_getinteger(v,3,(int64_t*)&format);
		}
	}
	else {
		time(&t);
	}
	tm *date;
	if(format == 'u')
		date = gmtime(&t);
	else
		date = localtime(&t);
	if(!date)
		return sq_throwerror(v,_SC("crt api failure"));
	sq_newtable(v);
	_set_integer_slot(v, _SC("sec"), date->tm_sec);
	_set_integer_slot(v, _SC("min"), date->tm_min);
	_set_integer_slot(v, _SC("hour"), date->tm_hour);
	_set_integer_slot(v, _SC("day"), date->tm_mday);
	_set_integer_slot(v, _SC("month"), date->tm_mon);
	_set_integer_slot(v, _SC("year"), date->tm_year+1900);
	_set_integer_slot(v, _SC("wday"), date->tm_wday);
	_set_integer_slot(v, _SC("yday"), date->tm_yday);
	return 1;
}



#define _DECL_FUNC(name,nparams,pmask) {_SC(#name),_system_##name,nparams,pmask}
static const rabbit::RegFunction systemlib_funcs[]={
	_DECL_FUNC(getenv,2,_SC(".s")),
	_DECL_FUNC(system,2,_SC(".s")),
	_DECL_FUNC(clock,0,NULL),
	_DECL_FUNC(time,1,NULL),
	_DECL_FUNC(date,-1,_SC(".nn")),
	_DECL_FUNC(remove,2,_SC(".s")),
	_DECL_FUNC(rename,3,_SC(".ss")),
	{NULL,(SQFUNCTION)0,0,NULL}
};
#undef _DECL_FUNC

int64_t rabbit::std::register_systemlib(rabbit::VirtualMachine* v)
{
	int64_t i=0;
	while(systemlib_funcs[i].name!=0)
	{
		sq_pushstring(v,systemlib_funcs[i].name,-1);
		sq_newclosure(v,systemlib_funcs[i].f,0);
		sq_setparamscheck(v,systemlib_funcs[i].nparamscheck,systemlib_funcs[i].typemask);
		sq_setnativeclosurename(v,-1,systemlib_funcs[i].name);
		sq_newslot(v,-3,SQFalse);
		i++;
	}
	return 1;
}
