/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/VirtualMachine.hpp>


#include <rabbit/Array.hpp>
#include <rabbit/SharedState.hpp>

#include <rabbit/String.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/Closure.hpp>
#include <rabbit/RegFunction.hpp>
#include <rabbit/NativeClosure.hpp>
#include <rabbit/FunctionProto.hpp>
#include <rabbit/Generator.hpp>


#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <rabbit/StackInfos.hpp>

static bool str2num(const char *s,rabbit::ObjectPtr &res,int64_t base)
{
	char *end;
	const char *e = s;
	bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
	bool isfloat = false;
	char c;
	while((c = *e) != '\0')
	{
		if (c == '.' || (!iseintbase && (c == 'E' || c == 'e'))) { //e and E is for scientific notation
			isfloat = true;
			break;
		}
		e++;
	}
	if(isfloat){
		float_t r = float_t(strtod(s,&end));
		if(s == end) return false;
		res = r;
	}
	else{
		int64_t r = int64_t(strtol(s,&end,(int)base));
		if(s == end) return false;
		res = r;
	}
	return true;
}

static int64_t base_dummy(rabbit::VirtualMachine* SQ_UNUSED_ARG(v))
{
	return 0;
}

static int64_t base_getroottable(rabbit::VirtualMachine* v)
{
	v->push(v->_roottable);
	return 1;
}

static int64_t base_getconsttable(rabbit::VirtualMachine* v)
{
	v->push(_get_shared_state(v)->_consts);
	return 1;
}


static int64_t base_setroottable(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = v->_roottable;
	if(SQ_FAILED(sq_setroottable(v))) return SQ_ERROR;
	v->push(o);
	return 1;
}

static int64_t base_setconsttable(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = _get_shared_state(v)->_consts;
	if(SQ_FAILED(sq_setconsttable(v))) return SQ_ERROR;
	v->push(o);
	return 1;
}

static int64_t base_seterrorhandler(rabbit::VirtualMachine* v)
{
	sq_seterrorhandler(v);
	return 0;
}

static int64_t base_setdebughook(rabbit::VirtualMachine* v)
{
	sq_setdebughook(v);
	return 0;
}

static int64_t base_enabledebuginfo(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &o=stack_get(v,2);

	sq_enabledebuginfo(v,rabbit::VirtualMachine::IsFalse(o)?SQFalse:SQTrue);
	return 0;
}

static int64_t __getcallstackinfos(rabbit::VirtualMachine* v,int64_t level)
{
	rabbit::StackInfos si;
	int64_t seq = 0;
	const char *name = NULL;

	if (SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
	{
		const char *fn = "unknown";
		const char *src = "unknown";
		if(si.funcname)fn = si.funcname;
		if(si.source)src = si.source;
		sq_newtable(v);
		sq_pushstring(v, "func", -1);
		sq_pushstring(v, fn, -1);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, "src", -1);
		sq_pushstring(v, src, -1);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, "line", -1);
		sq_pushinteger(v, si.line);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, "locals", -1);
		sq_newtable(v);
		seq=0;
		while ((name = sq_getlocal(v, level, seq))) {
			sq_pushstring(v, name, -1);
			sq_push(v, -2);
			sq_newslot(v, -4, SQFalse);
			sq_pop(v, 1);
			seq++;
		}
		sq_newslot(v, -3, SQFalse);
		return 1;
	}

	return 0;
}
static int64_t base_getstackinfos(rabbit::VirtualMachine* v)
{
	int64_t level;
	sq_getinteger(v, -1, &level);
	return __getcallstackinfos(v,level);
}

static int64_t base_assert(rabbit::VirtualMachine* v)
{
	if(rabbit::VirtualMachine::IsFalse(stack_get(v,2))){
		int64_t top = sq_gettop(v);
		if (top>2 && SQ_SUCCEEDED(sq_tostring(v,3))) {
			const char *str = 0;
			if (SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
				return sq_throwerror(v, str);
			}
		}
		return sq_throwerror(v, "assertion failed");
	}
	return 0;
}

static int64_t get_slice_params(rabbit::VirtualMachine* v,int64_t &sidx,int64_t &eidx,rabbit::ObjectPtr &o)
{
	int64_t top = sq_gettop(v);
	sidx=0;
	eidx=0;
	o=stack_get(v,1);
	if(top>1){
		rabbit::ObjectPtr &start=stack_get(v,2);
		if(sq_type(start)!=rabbit::OT_NULL && sq_isnumeric(start)){
			sidx=tointeger(start);
		}
	}
	if(top>2){
		rabbit::ObjectPtr &end=stack_get(v,3);
		if(sq_isnumeric(end)){
			eidx=tointeger(end);
		}
	}
	else {
		eidx = sq_getsize(v,1);
	}
	return 1;
}

static int64_t base_print(rabbit::VirtualMachine* v)
{
	const char *str;
	if(SQ_SUCCEEDED(sq_tostring(v,2)))
	{
		if(SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
			if(_get_shared_state(v)->_printfunc) _get_shared_state(v)->_printfunc(v,"%s",str);
			return 0;
		}
	}
	return SQ_ERROR;
}

static int64_t base_error(rabbit::VirtualMachine* v)
{
	const char *str;
	if(SQ_SUCCEEDED(sq_tostring(v,2)))
	{
		if(SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
			if(_get_shared_state(v)->_errorfunc) _get_shared_state(v)->_errorfunc(v,"%s",str);
			return 0;
		}
	}
	return SQ_ERROR;
}

static int64_t base_compilestring(rabbit::VirtualMachine* v)
{
	int64_t nargs=sq_gettop(v);
	const char *src=NULL,*name="unnamedbuffer";
	int64_t size;
	sq_getstring(v,2,&src);
	size=sq_getsize(v,2);
	if(nargs>2){
		sq_getstring(v,3,&name);
	}
	if(SQ_SUCCEEDED(sq_compilebuffer(v,src,size,name,SQFalse)))
		return 1;
	else
		return SQ_ERROR;
}

static int64_t base_newthread(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &func = stack_get(v,2);
	int64_t stksize = (func.toClosure()->_function->_stacksize << 1) +2;
	rabbit::VirtualMachine* newv = sq_newthread(v, (stksize < MIN_STACK_OVERHEAD + 2)? MIN_STACK_OVERHEAD + 2 : stksize);
	sq_move(newv,v,-2);
	return 1;
}

static int64_t base_suspend(rabbit::VirtualMachine* v)
{
	return sq_suspendvm(v);
}

static int64_t base_array(rabbit::VirtualMachine* v)
{
	rabbit::Array *a;
	rabbit::Object &size = stack_get(v,2);
	if(sq_gettop(v) > 2) {
		a = rabbit::Array::create(_get_shared_state(v),0);
		a->resize(tointeger(size),stack_get(v,3));
	}
	else {
		a = rabbit::Array::create(_get_shared_state(v),tointeger(size));
	}
	v->push(a);
	return 1;
}

static int64_t base_type(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &o = stack_get(v,2);
	v->push(rabbit::String::create(_get_shared_state(v),getTypeName(o),-1));
	return 1;
}

static int64_t base_callee(rabbit::VirtualMachine* v)
{
	if(v->_callsstacksize > 1)
	{
		v->push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return 1;
	}
	return sq_throwerror(v,"no closure in the calls stack");
}

static const rabbit::RegFunction base_funcs[]={
	//generic
	{"seterrorhandler",base_seterrorhandler,2, NULL},
	{"setdebughook",base_setdebughook,2, NULL},
	{"enabledebuginfo",base_enabledebuginfo,2, NULL},
	{"getstackinfos",base_getstackinfos,2, ".n"},
	{"getroottable",base_getroottable,1, NULL},
	{"setroottable",base_setroottable,2, NULL},
	{"getconsttable",base_getconsttable,1, NULL},
	{"setconsttable",base_setconsttable,2, NULL},
	{"assert",base_assert,-2, NULL},
	{"print",base_print,2, NULL},
	{"error",base_error,2, NULL},
	{"compilestring",base_compilestring,-2, ".ss"},
	{"newthread",base_newthread,2, ".c"},
	{"suspend",base_suspend,-1, NULL},
	{"array",base_array,-2, ".n"},
	{"type",base_type,2, NULL},
	{"callee",base_callee,0,NULL},
	{"dummy",base_dummy,0,NULL},
	{NULL,(SQFUNCTION)0,0,NULL}
};

void sq_base_register(rabbit::VirtualMachine* v)
{
	int64_t i=0;
	sq_pushroottable(v);
	while(base_funcs[i].name!=0) {
		sq_pushstring(v,base_funcs[i].name,-1);
		sq_newclosure(v,base_funcs[i].f,0);
		sq_setnativeclosurename(v,-1,base_funcs[i].name);
		sq_setparamscheck(v,base_funcs[i].nparamscheck,base_funcs[i].typemask);
		sq_newslot(v,-3, SQFalse);
		i++;
	}

	sq_pushstring(v,"_versionnumber_",-1);
	sq_pushinteger(v,RABBIT_VERSION_NUMBER);
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,"_version_",-1);
	sq_pushstring(v,RABBIT_VERSION,-1);
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,"_charsize_",-1);
	sq_pushinteger(v,sizeof(char));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,"_intsize_",-1);
	sq_pushinteger(v,sizeof(int64_t));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,"_floatsize_",-1);
	sq_pushinteger(v,sizeof(float_t));
	sq_newslot(v,-3, SQFalse);
	sq_pop(v,1);
}

static int64_t default_delegate_len(rabbit::VirtualMachine* v)
{
	v->push(int64_t(sq_getsize(v,1)));
	return 1;
}

static int64_t default_delegate_tofloat(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &o=stack_get(v,1);
	switch(sq_type(o)){
	case rabbit::OT_STRING:{
		rabbit::ObjectPtr res;
		if(str2num(_stringval(o),res,10)){
			v->push(rabbit::ObjectPtr(tofloat(res)));
			break;
		}}
		return sq_throwerror(v, "cannot convert the string");
		break;
	case rabbit::OT_INTEGER:
	case rabbit::OT_FLOAT:
		v->push(rabbit::ObjectPtr(tofloat(o)));
		break;
	case rabbit::OT_BOOL:
		v->push(rabbit::ObjectPtr((float_t)(o.toInteger()?1:0)));
		break;
	default:
		v->pushNull();
		break;
	}
	return 1;
}

static int64_t default_delegate_tointeger(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &o=stack_get(v,1);
	int64_t base = 10;
	if(sq_gettop(v) > 1) {
		sq_getinteger(v,2,&base);
	}
	switch(sq_type(o)){
	case rabbit::OT_STRING:{
		rabbit::ObjectPtr res;
		if(str2num(_stringval(o),res,base)){
			v->push(rabbit::ObjectPtr(tointeger(res)));
			break;
		}}
		return sq_throwerror(v, "cannot convert the string");
		break;
	case rabbit::OT_INTEGER:
	case rabbit::OT_FLOAT:
		v->push(rabbit::ObjectPtr(tointeger(o)));
		break;
	case rabbit::OT_BOOL:
		v->push(rabbit::ObjectPtr(o.toInteger()?(int64_t)1:(int64_t)0));
		break;
	default:
		v->pushNull();
		break;
	}
	return 1;
}

static int64_t default_delegate_tostring(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_tostring(v,1)))
		return SQ_ERROR;
	return 1;
}

static int64_t obj_delegate_weakref(rabbit::VirtualMachine* v)
{
	sq_weakref(v,1);
	return 1;
}

static int64_t obj_clear(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_clear(v,-1)) ? 1 : SQ_ERROR;
}


static int64_t number_delegate_tochar(rabbit::VirtualMachine* v)
{
	rabbit::Object &o=stack_get(v,1);
	char c = (char)tointeger(o);
	v->push(rabbit::String::create(_get_shared_state(v),(const char *)&c,1));
	return 1;
}



/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static int64_t table_rawdelete(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_rawdeleteslot(v,1,SQTrue)))
		return SQ_ERROR;
	return 1;
}


static int64_t container_rawexists(rabbit::VirtualMachine* v)
{
	if(SQ_SUCCEEDED(sq_rawget(v,-2))) {
		sq_pushbool(v,SQTrue);
		return 1;
	}
	sq_pushbool(v,SQFalse);
	return 1;
}

static int64_t container_rawset(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_rawset(v,-3)) ? 1 : SQ_ERROR;
}


static int64_t container_rawget(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_rawget(v,-2))?1:SQ_ERROR;
}

static int64_t table_setdelegate(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_setdelegate(v,-2)))
		return SQ_ERROR;
	sq_push(v,-1); // -1 because sq_setdelegate pops 1
	return 1;
}

static int64_t table_getdelegate(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_getdelegate(v,-1))?1:SQ_ERROR;
}

static int64_t table_filter(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	rabbit::Table *tbl = o.toTable();
	rabbit::ObjectPtr ret = rabbit::Table::create(_get_shared_state(v),0);

	rabbit::ObjectPtr itr, key, val;
	int64_t nitr;
	while((nitr = tbl->next(false, itr, key, val)) != -1) {
		itr = (int64_t)nitr;

		v->push(o);
		v->push(key);
		v->push(val);
		if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {
			return SQ_ERROR;
		}
		if(!rabbit::VirtualMachine::IsFalse(v->getUp(-1))) {
			ret.toTable()->newSlot(key, val);
		}
		v->pop();
	}

	v->push(ret);
	return 1;
}


const rabbit::RegFunction rabbit::SharedState::_table_default_delegate_funcz[]={
	{"len",default_delegate_len,1, "t"},
	{"rawget",container_rawget,2, "t"},
	{"rawset",container_rawset,3, "t"},
	{"rawdelete",table_rawdelete,2, "t"},
	{"rawin",container_rawexists,2, "t"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{"clear",obj_clear,1, "."},
	{"setdelegate",table_setdelegate,2, ".t|o"},
	{"getdelegate",table_getdelegate,1, "."},
	{"filter",table_filter,2, "tc"},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static int64_t array_append(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_arrayappend(v,-2)) ? 1 : SQ_ERROR;
}

static int64_t array_extend(rabbit::VirtualMachine* v)
{
	stack_get(v,1).toArray()->extend(stack_get(v,2).toArray());
	sq_pop(v,1);
	return 1;
}

static int64_t array_reverse(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_arrayreverse(v,-1)) ? 1 : SQ_ERROR;
}

static int64_t array_pop(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_arraypop(v,1,SQTrue))?1:SQ_ERROR;
}

static int64_t array_top(rabbit::VirtualMachine* v)
{
	rabbit::Object &o=stack_get(v,1);
	if(o.toArray()->size()>0){
		v->push(o.toArray()->top());
		return 1;
	}
	else return sq_throwerror(v,"top() on a empty array");
}

static int64_t array_insert(rabbit::VirtualMachine* v)
{
	rabbit::Object &o=stack_get(v,1);
	rabbit::Object &idx=stack_get(v,2);
	rabbit::Object &val=stack_get(v,3);
	if(!o.toArray()->insert(tointeger(idx),val))
		return sq_throwerror(v,"index out of range");
	sq_pop(v,2);
	return 1;
}

static int64_t array_remove(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v, 1);
	rabbit::Object &idx = stack_get(v, 2);
	if(!sq_isnumeric(idx)) return sq_throwerror(v, "wrong type");
	rabbit::ObjectPtr val;
	if(o.toArray()->get(tointeger(idx), val)) {
		o.toArray()->remove(tointeger(idx));
		v->push(val);
		return 1;
	}
	return sq_throwerror(v, "idx out of range");
}

static int64_t array_resize(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v, 1);
	rabbit::Object &nsize = stack_get(v, 2);
	rabbit::ObjectPtr fill;
	if(sq_isnumeric(nsize)) {
		int64_t sz = tointeger(nsize);
		if (sz<0)
		  return sq_throwerror(v, "resizing to negative length");

		if(sq_gettop(v) > 2)
			fill = stack_get(v, 3);
		o.toArray()->resize(sz,fill);
		sq_settop(v, 1);
		return 1;
	}
	return sq_throwerror(v, "size must be a number");
}

static int64_t __map_array(rabbit::Array *dest,rabbit::Array *src,rabbit::VirtualMachine* v) {
	rabbit::ObjectPtr temp;
	int64_t size = src->size();
	for(int64_t n = 0; n < size; n++) {
		src->get(n,temp);
		v->push(src);
		v->push(temp);
		if(SQ_FAILED(sq_call(v,2,SQTrue,SQFalse))) {
			return SQ_ERROR;
		}
		dest->set(n,v->getUp(-1));
		v->pop();
	}
	return 0;
}

static int64_t array_map(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	int64_t size = o.toArray()->size();
	rabbit::ObjectPtr ret = rabbit::Array::create(_get_shared_state(v),size);
	if(SQ_FAILED(__map_array(ret.toArray(),o.toArray(),v)))
		return SQ_ERROR;
	v->push(ret);
	return 1;
}

static int64_t array_apply(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	if(SQ_FAILED(__map_array(o.toArray(),o.toArray(),v)))
		return SQ_ERROR;
	sq_pop(v,1);
	return 1;
}

static int64_t array_reduce(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	rabbit::Array *a = o.toArray();
	int64_t size = a->size();
	if(size == 0) {
		return 0;
	}
	rabbit::ObjectPtr res;
	a->get(0,res);
	if(size > 1) {
		rabbit::ObjectPtr other;
		for(int64_t n = 1; n < size; n++) {
			a->get(n,other);
			v->push(o);
			v->push(res);
			v->push(other);
			if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {
				return SQ_ERROR;
			}
			res = v->getUp(-1);
			v->pop();
		}
	}
	v->push(res);
	return 1;
}

static int64_t array_filter(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	rabbit::Array *a = o.toArray();
	rabbit::ObjectPtr ret = rabbit::Array::create(_get_shared_state(v),0);
	int64_t size = a->size();
	rabbit::ObjectPtr val;
	for(int64_t n = 0; n < size; n++) {
		a->get(n,val);
		v->push(o);
		v->push(n);
		v->push(val);
		if(SQ_FAILED(sq_call(v,3,SQTrue,SQFalse))) {
			return SQ_ERROR;
		}
		if(!rabbit::VirtualMachine::IsFalse(v->getUp(-1))) {
			ret.toArray()->append(val);
		}
		v->pop();
	}
	v->push(ret);
	return 1;
}

static int64_t array_find(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	rabbit::ObjectPtr &val = stack_get(v,2);
	rabbit::Array *a = o.toArray();
	int64_t size = a->size();
	rabbit::ObjectPtr temp;
	for(int64_t n = 0; n < size; n++) {
		bool res = false;
		a->get(n,temp);
		if(rabbit::VirtualMachine::isEqual(temp,val,res) && res) {
			v->push(n);
			return 1;
		}
	}
	return 0;
}


static bool _sort_compare(rabbit::VirtualMachine* v,rabbit::ObjectPtr &a,rabbit::ObjectPtr &b,int64_t func,int64_t &ret)
{
	if(func < 0) {
		if(!v->objCmp(a,b,ret)) return false;
	}
	else {
		int64_t top = sq_gettop(v);
		sq_push(v, func);
		sq_pushroottable(v);
		v->push(a);
		v->push(b);
		if(SQ_FAILED(sq_call(v, 3, SQTrue, SQFalse))) {
			if(!sq_isstring( v->_lasterror))
				v->raise_error("compare func failed");
			return false;
		}
		if(SQ_FAILED(sq_getinteger(v, -1, &ret))) {
			v->raise_error("numeric value expected as return value of the compare function");
			return false;
		}
		sq_settop(v, top);
		return true;
	}
	return true;
}

static bool _hsort_sift_down(rabbit::VirtualMachine* v,rabbit::Array *arr, int64_t root, int64_t bottom, int64_t func)
{
	int64_t maxChild;
	int64_t done = 0;
	int64_t ret;
	int64_t root2;
	while (((root2 = root * 2) <= bottom) && (!done))
	{
		if (root2 == bottom) {
			maxChild = root2;
		}
		else {
			if(!_sort_compare(v,(*arr)[root2],(*arr)[root2 + 1],func,ret))
				return false;
			if (ret > 0) {
				maxChild = root2;
			}
			else {
				maxChild = root2 + 1;
			}
		}

		if(!_sort_compare(v,(*arr)[root],(*arr)[maxChild],func,ret))
			return false;
		if (ret < 0) {
			if (root == maxChild) {
				v->raise_error("inconsistent compare function");
				return false; // We'd be swapping ourselve. The compare function is incorrect
			}

			_Swap((*arr)[root], (*arr)[maxChild]);
			root = maxChild;
		}
		else {
			done = 1;
		}
	}
	return true;
}

static bool _hsort(rabbit::VirtualMachine* v,rabbit::ObjectPtr &arr, int64_t SQ_UNUSED_ARG(l), int64_t SQ_UNUSED_ARG(r),int64_t func)
{
	rabbit::Array *a = arr.toArray();
	int64_t i;
	int64_t array_size = a->size();
	for (i = (array_size / 2); i >= 0; i--) {
		if(!_hsort_sift_down(v,a, i, array_size - 1,func)) return false;
	}

	for (i = array_size-1; i >= 1; i--)
	{
		_Swap((*a)[0],(*a)[i]);
		if(!_hsort_sift_down(v,a, 0, i-1,func)) return false;
	}
	return true;
}

static int64_t array_sort(rabbit::VirtualMachine* v)
{
	int64_t func = -1;
	rabbit::ObjectPtr &o = stack_get(v,1);
	if(o.toArray()->size() > 1) {
		if(sq_gettop(v) == 2) func = 2;
		if(!_hsort(v, o, 0, o.toArray()->size()-1, func))
			return SQ_ERROR;

	}
	sq_settop(v,1);
	return 1;
}

static int64_t array_slice(rabbit::VirtualMachine* v)
{
	int64_t sidx,eidx;
	rabbit::ObjectPtr o;
	if(get_slice_params(v,sidx,eidx,o)==-1)return -1;
	int64_t alen = o.toArray()->size();
	if(sidx < 0)sidx = alen + sidx;
	if(eidx < 0)eidx = alen + eidx;
	if(eidx < sidx)return sq_throwerror(v,"wrong indexes");
	if(eidx > alen || sidx < 0)return sq_throwerror(v, "slice out of range");
	rabbit::Array *arr=rabbit::Array::create(_get_shared_state(v),eidx-sidx);
	rabbit::ObjectPtr t;
	int64_t count=0;
	for(int64_t i=sidx;i<eidx;i++){
		o.toArray()->get(i,t);
		arr->set(count++,t);
	}
	v->push(arr);
	return 1;

}

const rabbit::RegFunction rabbit::SharedState::_array_default_delegate_funcz[]={
	{"len",default_delegate_len,1, "a"},
	{"append",array_append,2, "a"},
	{"extend",array_extend,2, "aa"},
	{"push",array_append,2, "a"},
	{"pop",array_pop,1, "a"},
	{"top",array_top,1, "a"},
	{"insert",array_insert,3, "an"},
	{"remove",array_remove,2, "an"},
	{"resize",array_resize,-2, "an"},
	{"reverse",array_reverse,1, "a"},
	{"sort",array_sort,-1, "ac"},
	{"slice",array_slice,-1, "ann"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{"clear",obj_clear,1, "."},
	{"map",array_map,2, "ac"},
	{"apply",array_apply,2, "ac"},
	{"reduce",array_reduce,2, "ac"},
	{"filter",array_filter,2, "ac"},
	{"find",array_find,2, "a."},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//STRING DEFAULT DELEGATE//////////////////////////
static int64_t string_slice(rabbit::VirtualMachine* v)
{
	int64_t sidx,eidx;
	rabbit::ObjectPtr o;
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
	int64_t slen = o.toString()->_len;
	if(sidx < 0)sidx = slen + sidx;
	if(eidx < 0)eidx = slen + eidx;
	if(eidx < sidx) return sq_throwerror(v,"wrong indexes");
	if(eidx > slen || sidx < 0) return sq_throwerror(v, "slice out of range");
	v->push(rabbit::String::create(_get_shared_state(v),&_stringval(o)[sidx],eidx-sidx));
	return 1;
}

static int64_t string_find(rabbit::VirtualMachine* v)
{
	int64_t top,start_idx=0;
	const char *str,*substr,*ret;
	if(((top=sq_gettop(v))>1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))){
		if(top>2)sq_getinteger(v,3,&start_idx);
		if((sq_getsize(v,1)>start_idx) && (start_idx>=0)){
			ret=strstr(&str[start_idx],substr);
			if(ret){
				sq_pushinteger(v,(int64_t)(ret-str));
				return 1;
			}
		}
		return 0;
	}
	return sq_throwerror(v,"invalid param");
}

#define STRING_TOFUNCZ(func) static int64_t string_##func(rabbit::VirtualMachine* v) \
{\
	int64_t sidx,eidx; \
	rabbit::ObjectPtr str; \
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,str)))return -1; \
	int64_t slen = str.toString()->_len; \
	if(sidx < 0)sidx = slen + sidx; \
	if(eidx < 0)eidx = slen + eidx; \
	if(eidx < sidx) return sq_throwerror(v,"wrong indexes"); \
	if(eidx > slen || sidx < 0) return sq_throwerror(v,"slice out of range"); \
	int64_t len=str.toString()->_len; \
	const char *sthis=_stringval(str); \
	char *snew=(_get_shared_state(v)->getScratchPad(sq_rsl(len))); \
	memcpy(snew,sthis,sq_rsl(len));\
	for(int64_t i=sidx;i<eidx;i++) snew[i] = func(sthis[i]); \
	v->push(rabbit::String::create(_get_shared_state(v),snew,len)); \
	return 1; \
}


STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

const rabbit::RegFunction rabbit::SharedState::_string_default_delegate_funcz[]={
	{"len",default_delegate_len,1, "s"},
	{"tointeger",default_delegate_tointeger,-1, "sn"},
	{"tofloat",default_delegate_tofloat,1, "s"},
	{"tostring",default_delegate_tostring,1, "."},
	{"slice",string_slice,-1, "s n  n"},
	{"find",string_find,-2, "s s n"},
	{"tolower",string_tolower,-1, "s n n"},
	{"toupper",string_toupper,-1, "s n n"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{NULL,(SQFUNCTION)0,0,NULL}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
const rabbit::RegFunction rabbit::SharedState::_number_default_delegate_funcz[]={
	{"tointeger",default_delegate_tointeger,1, "n|b"},
	{"tofloat",default_delegate_tofloat,1, "n|b"},
	{"tostring",default_delegate_tostring,1, "."},
	{"tochar",number_delegate_tochar,1, "n|b"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{NULL,(SQFUNCTION)0,0,NULL}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
static int64_t closure_pcall(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_call(v,sq_gettop(v)-1,SQTrue,SQFalse))?1:SQ_ERROR;
}

static int64_t closure_call(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &c = stack_get(v, -1);
	if (sq_type(c) == rabbit::OT_CLOSURE && (c.toClosure()->_function->_bgenerator == false))
	{
		return sq_tailcall(v, sq_gettop(v) - 1);
	}
	return SQ_SUCCEEDED(sq_call(v, sq_gettop(v) - 1, SQTrue, SQTrue)) ? 1 : SQ_ERROR;
}

static int64_t _closure_acall(rabbit::VirtualMachine* v,rabbit::Bool raiseerror)
{
	rabbit::Array *aparams=stack_get(v,2).toArray();
	int64_t nparams=aparams->size();
	v->push(stack_get(v,1));
	for(int64_t i=0;i<nparams;i++)v->push((*aparams)[i]);
	return SQ_SUCCEEDED(sq_call(v,nparams,SQTrue,raiseerror))?1:SQ_ERROR;
}

static int64_t closure_acall(rabbit::VirtualMachine* v)
{
	return _closure_acall(v,SQTrue);
}

static int64_t closure_pacall(rabbit::VirtualMachine* v)
{
	return _closure_acall(v,SQFalse);
}

static int64_t closure_bindenv(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_bindenv(v,1)))
		return SQ_ERROR;
	return 1;
}

static int64_t closure_getroot(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_getclosureroot(v,-1)))
		return SQ_ERROR;
	return 1;
}

static int64_t closure_setroot(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_setclosureroot(v,-2)))
		return SQ_ERROR;
	return 1;
}

static int64_t closure_getinfos(rabbit::VirtualMachine* v) {
	rabbit::Object o = stack_get(v,1);
	rabbit::Table *res = rabbit::Table::create(_get_shared_state(v),4);
	if(sq_type(o) == rabbit::OT_CLOSURE) {
		rabbit::FunctionProto *f = o.toClosure()->_function;
		int64_t nparams = f->_nparameters + (f->_varparams?1:0);
		rabbit::ObjectPtr params = rabbit::Array::create(_get_shared_state(v),nparams);
	rabbit::ObjectPtr defparams = rabbit::Array::create(_get_shared_state(v),f->_ndefaultparams);
		for(int64_t n = 0; n<f->_nparameters; n++) {
			params.toArray()->set((int64_t)n,f->_parameters[n]);
		}
	for(int64_t j = 0; j<f->_ndefaultparams; j++) {
			defparams.toArray()->set((int64_t)j,o.toClosure()->_defaultparams[j]);
		}
		if(f->_varparams) {
			params.toArray()->set(nparams-1,rabbit::String::create(_get_shared_state(v),"...",-1));
		}
		res->newSlot(rabbit::String::create(_get_shared_state(v),"native",-1),false);
		res->newSlot(rabbit::String::create(_get_shared_state(v),"name",-1),f->_name);
		res->newSlot(rabbit::String::create(_get_shared_state(v),"src",-1),f->_sourcename);
		res->newSlot(rabbit::String::create(_get_shared_state(v),"parameters",-1),params);
		res->newSlot(rabbit::String::create(_get_shared_state(v),"varargs",-1),f->_varparams);
	res->newSlot(rabbit::String::create(_get_shared_state(v),"defparams",-1),defparams);
	}
	else { //rabbit::OT_NATIVECLOSURE
		rabbit::NativeClosure *nc = o.toNativeClosure();
		res->newSlot(rabbit::String::create(_get_shared_state(v),"native",-1),true);
		res->newSlot(rabbit::String::create(_get_shared_state(v),"name",-1),nc->_name);
		res->newSlot(rabbit::String::create(_get_shared_state(v),"paramscheck",-1),nc->_nparamscheck);
		rabbit::ObjectPtr typecheck;
		if(nc->_typecheck.size() > 0) {
			typecheck =
				rabbit::Array::create(_get_shared_state(v), nc->_typecheck.size());
			for(uint64_t n = 0; n<nc->_typecheck.size(); n++) {
					typecheck.toArray()->set((int64_t)n,nc->_typecheck[n]);
			}
		}
		res->newSlot(rabbit::String::create(_get_shared_state(v),"typecheck",-1),typecheck);
	}
	v->push(res);
	return 1;
}



const rabbit::RegFunction rabbit::SharedState::_closure_default_delegate_funcz[]={
	{"call",closure_call,-1, "c"},
	{"pcall",closure_pcall,-1, "c"},
	{"acall",closure_acall,2, "ca"},
	{"pacall",closure_pacall,2, "ca"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{"bindenv",closure_bindenv,2, "c x|y|t"},
	{"getinfos",closure_getinfos,1, "c"},
	{"getroot",closure_getroot,1, "c"},
	{"setroot",closure_setroot,2, "ct"},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//GENERATOR DEFAULT DELEGATE
static int64_t generator_getstatus(rabbit::VirtualMachine* v)
{
	rabbit::Object &o=stack_get(v,1);
	switch(o.toGenerator()->_state){
		case rabbit::Generator::eSuspended:v->push(rabbit::String::create(_get_shared_state(v),"suspended"));break;
		case rabbit::Generator::eRunning:v->push(rabbit::String::create(_get_shared_state(v),"running"));break;
		case rabbit::Generator::eDead:v->push(rabbit::String::create(_get_shared_state(v),"dead"));break;
	}
	return 1;
}

const rabbit::RegFunction rabbit::SharedState::_generator_default_delegate_funcz[]={
	{"getstatus",generator_getstatus,1, "g"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//THREAD DEFAULT DELEGATE
static int64_t thread_call(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		int64_t nparams = sq_gettop(v);
		o.toVirtualMachine()->push(o.toVirtualMachine()->_roottable);
		for(int64_t i = 2; i<(nparams+1); i++)
			sq_move(o.toVirtualMachine(),v,i);
		if(SQ_SUCCEEDED(sq_call(o.toVirtualMachine(),nparams,SQTrue,SQTrue))) {
			sq_move(v,o.toVirtualMachine(),-1);
			sq_pop(o.toVirtualMachine(),1);
			return 1;
		}
		v->_lasterror = o.toVirtualMachine()->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,"wrong parameter");
}

static int64_t thread_wakeup(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		rabbit::VirtualMachine *thread = o.toVirtualMachine();
		int64_t state = sq_getvmstate(thread);
		if(state != SQ_VMSTATE_SUSPENDED) {
			switch(state) {
				case SQ_VMSTATE_IDLE:
					return sq_throwerror(v,"cannot wakeup a idle thread");
				break;
				case SQ_VMSTATE_RUNNING:
					return sq_throwerror(v,"cannot wakeup a running thread");
				break;
			}
		}

		int64_t wakeupret = sq_gettop(v)>1?SQTrue:SQFalse;
		if(wakeupret) {
			sq_move(thread,v,2);
		}
		if(SQ_SUCCEEDED(sq_wakeupvm(thread,wakeupret,SQTrue,SQTrue,SQFalse))) {
			sq_move(v,thread,-1);
			sq_pop(thread,1); //pop retval
			if(sq_getvmstate(thread) == SQ_VMSTATE_IDLE) {
				sq_settop(thread,1); //pop roottable
			}
			return 1;
		}
		sq_settop(thread,1);
		v->_lasterror = thread->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,"wrong parameter");
}

static int64_t thread_wakeupthrow(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		rabbit::VirtualMachine *thread = o.toVirtualMachine();
		int64_t state = sq_getvmstate(thread);
		if(state != SQ_VMSTATE_SUSPENDED) {
			switch(state) {
				case SQ_VMSTATE_IDLE:
					return sq_throwerror(v,"cannot wakeup a idle thread");
				break;
				case SQ_VMSTATE_RUNNING:
					return sq_throwerror(v,"cannot wakeup a running thread");
				break;
			}
		}

		sq_move(thread,v,2);
		sq_throwobject(thread);
		rabbit::Bool rethrow_error = SQTrue;
		if(sq_gettop(v) > 2) {
			sq_getbool(v,3,&rethrow_error);
		}
		if(SQ_SUCCEEDED(sq_wakeupvm(thread,SQFalse,SQTrue,SQTrue,SQTrue))) {
			sq_move(v,thread,-1);
			sq_pop(thread,1); //pop retval
			if(sq_getvmstate(thread) == SQ_VMSTATE_IDLE) {
				sq_settop(thread,1); //pop roottable
			}
			return 1;
		}
		sq_settop(thread,1);
		if(rethrow_error) {
			v->_lasterror = thread->_lasterror;
			return SQ_ERROR;
		}
		return SQ_OK;
	}
	return sq_throwerror(v,"wrong parameter");
}

static int64_t thread_getstatus(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &o = stack_get(v,1);
	switch(sq_getvmstate(o.toVirtualMachine())) {
		case SQ_VMSTATE_IDLE:
			sq_pushstring(v,"idle",-1);
		break;
		case SQ_VMSTATE_RUNNING:
			sq_pushstring(v,"running",-1);
		break;
		case SQ_VMSTATE_SUSPENDED:
			sq_pushstring(v,"suspended",-1);
		break;
		default:
			return sq_throwerror(v,"internal VM error");
	}
	return 1;
}

static int64_t thread_getstackinfos(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		rabbit::VirtualMachine *thread = o.toVirtualMachine();
		int64_t threadtop = sq_gettop(thread);
		int64_t level;
		sq_getinteger(v,-1,&level);
		rabbit::Result res = __getcallstackinfos(thread,level);
		if(SQ_FAILED(res))
		{
			sq_settop(thread,threadtop);
			if(sq_type(thread->_lasterror) == rabbit::OT_STRING) {
				sq_throwerror(v,_stringval(thread->_lasterror));
			}
			else {
				sq_throwerror(v,"unknown error");
			}
		}
		if(res > 0) {
			//some result
			sq_move(v,thread,-1);
			sq_settop(thread,threadtop);
			return 1;
		}
		//no result
		sq_settop(thread,threadtop);
		return 0;

	}
	return sq_throwerror(v,"wrong parameter");
}

const rabbit::RegFunction rabbit::SharedState::_thread_default_delegate_funcz[] = {
	{"call", thread_call, -1, "v"},
	{"wakeup", thread_wakeup, -1, "v"},
	{"wakeupthrow", thread_wakeupthrow, -2, "v.b"},
	{"getstatus", thread_getstatus, 1, "v"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"getstackinfos",thread_getstackinfos,2, "vn"},
	{"tostring",default_delegate_tostring,1, "."},
	{NULL,(SQFUNCTION)0,0,NULL}
};

static int64_t class_getattributes(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_getattributes(v,-2))?1:SQ_ERROR;
}

static int64_t class_setattributes(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_setattributes(v,-3))?1:SQ_ERROR;
}

static int64_t class_instance(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_createinstance(v,-1))?1:SQ_ERROR;
}

static int64_t class_getbase(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_getbase(v,-1))?1:SQ_ERROR;
}

static int64_t class_newmember(rabbit::VirtualMachine* v)
{
	int64_t top = sq_gettop(v);
	rabbit::Bool bstatic = SQFalse;
	if(top == 5)
	{
		sq_tobool(v,-1,&bstatic);
		sq_pop(v,1);
	}

	if(top < 4) {
		sq_pushnull(v);
	}
	return SQ_SUCCEEDED(sq_newmember(v,-4,bstatic))?1:SQ_ERROR;
}

static int64_t class_rawnewmember(rabbit::VirtualMachine* v)
{
	int64_t top = sq_gettop(v);
	rabbit::Bool bstatic = SQFalse;
	if(top == 5)
	{
		sq_tobool(v,-1,&bstatic);
		sq_pop(v,1);
	}

	if(top < 4) {
		sq_pushnull(v);
	}
	return SQ_SUCCEEDED(sq_rawnewmember(v,-4,bstatic))?1:SQ_ERROR;
}

const rabbit::RegFunction rabbit::SharedState::_class_default_delegate_funcz[] = {
	{"getattributes", class_getattributes, 2, "y."},
	{"setattributes", class_setattributes, 3, "y.."},
	{"rawget",container_rawget,2, "y"},
	{"rawset",container_rawset,3, "y"},
	{"rawin",container_rawexists,2, "y"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{"instance",class_instance,1, "y"},
	{"getbase",class_getbase,1, "y"},
	{"newmember",class_newmember,-3, "y"},
	{"rawnewmember",class_rawnewmember,-3, "y"},
	{NULL,(SQFUNCTION)0,0,NULL}
};


static int64_t instance_getclass(rabbit::VirtualMachine* v)
{
	if(SQ_SUCCEEDED(sq_getclass(v,1)))
		return 1;
	return SQ_ERROR;
}

const rabbit::RegFunction rabbit::SharedState::_instance_default_delegate_funcz[] = {
	{"getclass", instance_getclass, 1, "x"},
	{"rawget",container_rawget,2, "x"},
	{"rawset",container_rawset,3, "x"},
	{"rawin",container_rawexists,2, "x"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{NULL,(SQFUNCTION)0,0,NULL}
};

static int64_t weakref_ref(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_getweakrefval(v,1)))
		return SQ_ERROR;
	return 1;
}

const rabbit::RegFunction rabbit::SharedState::_weakref_default_delegate_funcz[] = {
	{"ref",weakref_ref,1, "r"},
	{"weakref",obj_delegate_weakref,1, NULL },
	{"tostring",default_delegate_tostring,1, "."},
	{NULL,(SQFUNCTION)0,0,NULL}
};
