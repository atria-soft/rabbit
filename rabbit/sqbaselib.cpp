/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/sqpcheader.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/sqstring.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/Array.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/sqclass.hpp>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

static bool str2num(const rabbit::Char *s,rabbit::ObjectPtr &res,int64_t base)
{
	rabbit::Char *end;
	const rabbit::Char *e = s;
	bool iseintbase = base > 13; //to fix error converting hexadecimals with e like 56f0791e
	bool isfloat = false;
	rabbit::Char c;
	while((c = *e) != _SC('\0'))
	{
		if (c == _SC('.') || (!iseintbase && (c == _SC('E') || c == _SC('e')))) { //e and E is for scientific notation
			isfloat = true;
			break;
		}
		e++;
	}
	if(isfloat){
		float_t r = float_t(scstrtod(s,&end));
		if(s == end) return false;
		res = r;
	}
	else{
		int64_t r = int64_t(scstrtol(s,&end,(int)base));
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
	const rabbit::Char *name = NULL;

	if (SQ_SUCCEEDED(sq_stackinfos(v, level, &si)))
	{
		const rabbit::Char *fn = _SC("unknown");
		const rabbit::Char *src = _SC("unknown");
		if(si.funcname)fn = si.funcname;
		if(si.source)src = si.source;
		sq_newtable(v);
		sq_pushstring(v, _SC("func"), -1);
		sq_pushstring(v, fn, -1);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("src"), -1);
		sq_pushstring(v, src, -1);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("line"), -1);
		sq_pushinteger(v, si.line);
		sq_newslot(v, -3, SQFalse);
		sq_pushstring(v, _SC("locals"), -1);
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
			const rabbit::Char *str = 0;
			if (SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
				return sq_throwerror(v, str);
			}
		}
		return sq_throwerror(v, _SC("assertion failed"));
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
	const rabbit::Char *str;
	if(SQ_SUCCEEDED(sq_tostring(v,2)))
	{
		if(SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
			if(_get_shared_state(v)->_printfunc) _ss(v)->_printfunc(v,_SC("%s"),str);
			return 0;
		}
	}
	return SQ_ERROR;
}

static int64_t base_error(rabbit::VirtualMachine* v)
{
	const rabbit::Char *str;
	if(SQ_SUCCEEDED(sq_tostring(v,2)))
	{
		if(SQ_SUCCEEDED(sq_getstring(v,-1,&str))) {
			if(_get_shared_state(v)->_errorfunc) _ss(v)->_errorfunc(v,_SC("%s"),str);
			return 0;
		}
	}
	return SQ_ERROR;
}

static int64_t base_compilestring(rabbit::VirtualMachine* v)
{
	int64_t nargs=sq_gettop(v);
	const rabbit::Char *src=NULL,*name=_SC("unnamedbuffer");
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
	int64_t stksize = (_closure(func)->_function->_stacksize << 1) +2;
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
	v->push(SQString::create(_get_shared_state(v),getTypeName(o),-1));
	return 1;
}

static int64_t base_callee(rabbit::VirtualMachine* v)
{
	if(v->_callsstacksize > 1)
	{
		v->push(v->_callsstack[v->_callsstacksize - 2]._closure);
		return 1;
	}
	return sq_throwerror(v,_SC("no closure in the calls stack"));
}

static const rabbit::RegFunction base_funcs[]={
	//generic
	{_SC("seterrorhandler"),base_seterrorhandler,2, NULL},
	{_SC("setdebughook"),base_setdebughook,2, NULL},
	{_SC("enabledebuginfo"),base_enabledebuginfo,2, NULL},
	{_SC("getstackinfos"),base_getstackinfos,2, _SC(".n")},
	{_SC("getroottable"),base_getroottable,1, NULL},
	{_SC("setroottable"),base_setroottable,2, NULL},
	{_SC("getconsttable"),base_getconsttable,1, NULL},
	{_SC("setconsttable"),base_setconsttable,2, NULL},
	{_SC("assert"),base_assert,-2, NULL},
	{_SC("print"),base_print,2, NULL},
	{_SC("error"),base_error,2, NULL},
	{_SC("compilestring"),base_compilestring,-2, _SC(".ss")},
	{_SC("newthread"),base_newthread,2, _SC(".c")},
	{_SC("suspend"),base_suspend,-1, NULL},
	{_SC("array"),base_array,-2, _SC(".n")},
	{_SC("type"),base_type,2, NULL},
	{_SC("callee"),base_callee,0,NULL},
	{_SC("dummy"),base_dummy,0,NULL},
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

	sq_pushstring(v,_SC("_versionnumber_"),-1);
	sq_pushinteger(v,RABBIT_VERSION_NUMBER);
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_version_"),-1);
	sq_pushstring(v,RABBIT_VERSION,-1);
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_charsize_"),-1);
	sq_pushinteger(v,sizeof(rabbit::Char));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_intsize_"),-1);
	sq_pushinteger(v,sizeof(int64_t));
	sq_newslot(v,-3, SQFalse);
	sq_pushstring(v,_SC("_floatsize_"),-1);
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
		return sq_throwerror(v, _SC("cannot convert the string"));
		break;
	case rabbit::OT_INTEGER:case OT_FLOAT:
		v->push(rabbit::ObjectPtr(tofloat(o)));
		break;
	case rabbit::OT_BOOL:
		v->push(rabbit::ObjectPtr((float_t)(_integer(o)?1:0)));
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
		return sq_throwerror(v, _SC("cannot convert the string"));
		break;
	case rabbit::OT_INTEGER:case OT_FLOAT:
		v->push(rabbit::ObjectPtr(tointeger(o)));
		break;
	case rabbit::OT_BOOL:
		v->push(rabbit::ObjectPtr(_integer(o)?(int64_t)1:(int64_t)0));
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
	rabbit::Char c = (rabbit::Char)tointeger(o);
	v->push(SQString::create(_get_shared_state(v),(const rabbit::Char *)&c,1));
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
	SQTable *tbl = _table(o);
	rabbit::ObjectPtr ret = SQTable::create(_get_shared_state(v),0);

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
			_table(ret)->newSlot(key, val);
		}
		v->pop();
	}

	v->push(ret);
	return 1;
}


const rabbit::RegFunction SQSharedState::_table_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("t")},
	{_SC("rawget"),container_rawget,2, _SC("t")},
	{_SC("rawset"),container_rawset,3, _SC("t")},
	{_SC("rawdelete"),table_rawdelete,2, _SC("t")},
	{_SC("rawin"),container_rawexists,2, _SC("t")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("clear"),obj_clear,1, _SC(".")},
	{_SC("setdelegate"),table_setdelegate,2, _SC(".t|o")},
	{_SC("getdelegate"),table_getdelegate,1, _SC(".")},
	{_SC("filter"),table_filter,2, _SC("tc")},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static int64_t array_append(rabbit::VirtualMachine* v)
{
	return SQ_SUCCEEDED(sq_arrayappend(v,-2)) ? 1 : SQ_ERROR;
}

static int64_t array_extend(rabbit::VirtualMachine* v)
{
	_array(stack_get(v,1))->extend(_array(stack_get(v,2)));
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
	if(_array(o)->size()>0){
		v->push(_array(o)->top());
		return 1;
	}
	else return sq_throwerror(v,_SC("top() on a empty array"));
}

static int64_t array_insert(rabbit::VirtualMachine* v)
{
	rabbit::Object &o=stack_get(v,1);
	rabbit::Object &idx=stack_get(v,2);
	rabbit::Object &val=stack_get(v,3);
	if(!_array(o)->insert(tointeger(idx),val))
		return sq_throwerror(v,_SC("index out of range"));
	sq_pop(v,2);
	return 1;
}

static int64_t array_remove(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v, 1);
	rabbit::Object &idx = stack_get(v, 2);
	if(!sq_isnumeric(idx)) return sq_throwerror(v, _SC("wrong type"));
	rabbit::ObjectPtr val;
	if(_array(o)->get(tointeger(idx), val)) {
		_array(o)->remove(tointeger(idx));
		v->push(val);
		return 1;
	}
	return sq_throwerror(v, _SC("idx out of range"));
}

static int64_t array_resize(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v, 1);
	rabbit::Object &nsize = stack_get(v, 2);
	rabbit::ObjectPtr fill;
	if(sq_isnumeric(nsize)) {
		int64_t sz = tointeger(nsize);
		if (sz<0)
		  return sq_throwerror(v, _SC("resizing to negative length"));

		if(sq_gettop(v) > 2)
			fill = stack_get(v, 3);
		_array(o)->resize(sz,fill);
		sq_settop(v, 1);
		return 1;
	}
	return sq_throwerror(v, _SC("size must be a number"));
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
	int64_t size = _array(o)->size();
	rabbit::ObjectPtr ret = rabbit::Array::create(_get_shared_state(v),size);
	if(SQ_FAILED(__map_array(_array(ret),_array(o),v)))
		return SQ_ERROR;
	v->push(ret);
	return 1;
}

static int64_t array_apply(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	if(SQ_FAILED(__map_array(_array(o),_array(o),v)))
		return SQ_ERROR;
	sq_pop(v,1);
	return 1;
}

static int64_t array_reduce(rabbit::VirtualMachine* v)
{
	rabbit::Object &o = stack_get(v,1);
	rabbit::Array *a = _array(o);
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
	rabbit::Array *a = _array(o);
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
			_array(ret)->append(val);
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
	rabbit::Array *a = _array(o);
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
				v->raise_error(_SC("compare func failed"));
			return false;
		}
		if(SQ_FAILED(sq_getinteger(v, -1, &ret))) {
			v->raise_error(_SC("numeric value expected as return value of the compare function"));
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
				v->raise_error(_SC("inconsistent compare function"));
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
	rabbit::Array *a = _array(arr);
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
	if(_array(o)->size() > 1) {
		if(sq_gettop(v) == 2) func = 2;
		if(!_hsort(v, o, 0, _array(o)->size()-1, func))
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
	int64_t alen = _array(o)->size();
	if(sidx < 0)sidx = alen + sidx;
	if(eidx < 0)eidx = alen + eidx;
	if(eidx < sidx)return sq_throwerror(v,_SC("wrong indexes"));
	if(eidx > alen || sidx < 0)return sq_throwerror(v, _SC("slice out of range"));
	rabbit::Array *arr=rabbit::Array::create(_get_shared_state(v),eidx-sidx);
	rabbit::ObjectPtr t;
	int64_t count=0;
	for(int64_t i=sidx;i<eidx;i++){
		_array(o)->get(i,t);
		arr->set(count++,t);
	}
	v->push(arr);
	return 1;

}

const rabbit::RegFunction SQSharedState::_array_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("a")},
	{_SC("append"),array_append,2, _SC("a")},
	{_SC("extend"),array_extend,2, _SC("aa")},
	{_SC("push"),array_append,2, _SC("a")},
	{_SC("pop"),array_pop,1, _SC("a")},
	{_SC("top"),array_top,1, _SC("a")},
	{_SC("insert"),array_insert,3, _SC("an")},
	{_SC("remove"),array_remove,2, _SC("an")},
	{_SC("resize"),array_resize,-2, _SC("an")},
	{_SC("reverse"),array_reverse,1, _SC("a")},
	{_SC("sort"),array_sort,-1, _SC("ac")},
	{_SC("slice"),array_slice,-1, _SC("ann")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("clear"),obj_clear,1, _SC(".")},
	{_SC("map"),array_map,2, _SC("ac")},
	{_SC("apply"),array_apply,2, _SC("ac")},
	{_SC("reduce"),array_reduce,2, _SC("ac")},
	{_SC("filter"),array_filter,2, _SC("ac")},
	{_SC("find"),array_find,2, _SC("a.")},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//STRING DEFAULT DELEGATE//////////////////////////
static int64_t string_slice(rabbit::VirtualMachine* v)
{
	int64_t sidx,eidx;
	rabbit::ObjectPtr o;
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
	int64_t slen = _string(o)->_len;
	if(sidx < 0)sidx = slen + sidx;
	if(eidx < 0)eidx = slen + eidx;
	if(eidx < sidx) return sq_throwerror(v,_SC("wrong indexes"));
	if(eidx > slen || sidx < 0) return sq_throwerror(v, _SC("slice out of range"));
	v->push(SQString::create(_get_shared_state(v),&_stringval(o)[sidx],eidx-sidx));
	return 1;
}

static int64_t string_find(rabbit::VirtualMachine* v)
{
	int64_t top,start_idx=0;
	const rabbit::Char *str,*substr,*ret;
	if(((top=sq_gettop(v))>1) && SQ_SUCCEEDED(sq_getstring(v,1,&str)) && SQ_SUCCEEDED(sq_getstring(v,2,&substr))){
		if(top>2)sq_getinteger(v,3,&start_idx);
		if((sq_getsize(v,1)>start_idx) && (start_idx>=0)){
			ret=scstrstr(&str[start_idx],substr);
			if(ret){
				sq_pushinteger(v,(int64_t)(ret-str));
				return 1;
			}
		}
		return 0;
	}
	return sq_throwerror(v,_SC("invalid param"));
}

#define STRING_TOFUNCZ(func) static int64_t string_##func(rabbit::VirtualMachine* v) \
{\
	int64_t sidx,eidx; \
	rabbit::ObjectPtr str; \
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,str)))return -1; \
	int64_t slen = _string(str)->_len; \
	if(sidx < 0)sidx = slen + sidx; \
	if(eidx < 0)eidx = slen + eidx; \
	if(eidx < sidx) return sq_throwerror(v,_SC("wrong indexes")); \
	if(eidx > slen || sidx < 0) return sq_throwerror(v,_SC("slice out of range")); \
	int64_t len=_string(str)->_len; \
	const rabbit::Char *sthis=_stringval(str); \
	rabbit::Char *snew=(_get_shared_state(v)->getScratchPad(sq_rsl(len))); \
	memcpy(snew,sthis,sq_rsl(len));\
	for(int64_t i=sidx;i<eidx;i++) snew[i] = func(sthis[i]); \
	v->push(SQString::create(_get_shared_state(v),snew,len)); \
	return 1; \
}


STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

const rabbit::RegFunction SQSharedState::_string_default_delegate_funcz[]={
	{_SC("len"),default_delegate_len,1, _SC("s")},
	{_SC("tointeger"),default_delegate_tointeger,-1, _SC("sn")},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("s")},
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("slice"),string_slice,-1, _SC("s n  n")},
	{_SC("find"),string_find,-2, _SC("s s n")},
	{_SC("tolower"),string_tolower,-1, _SC("s n n")},
	{_SC("toupper"),string_toupper,-1, _SC("s n n")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{NULL,(SQFUNCTION)0,0,NULL}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
const rabbit::RegFunction SQSharedState::_number_default_delegate_funcz[]={
	{_SC("tointeger"),default_delegate_tointeger,1, _SC("n|b")},
	{_SC("tofloat"),default_delegate_tofloat,1, _SC("n|b")},
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("tochar"),number_delegate_tochar,1, _SC("n|b")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
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
	if (sq_type(c) == rabbit::OT_CLOSURE && (_closure(c)->_function->_bgenerator == false))
	{
		return sq_tailcall(v, sq_gettop(v) - 1);
	}
	return SQ_SUCCEEDED(sq_call(v, sq_gettop(v) - 1, SQTrue, SQTrue)) ? 1 : SQ_ERROR;
}

static int64_t _closure_acall(rabbit::VirtualMachine* v,rabbit::Bool raiseerror)
{
	rabbit::Array *aparams=_array(stack_get(v,2));
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
	SQTable *res = SQTable::create(_get_shared_state(v),4);
	if(sq_type(o) == rabbit::OT_CLOSURE) {
		SQFunctionProto *f = _closure(o)->_function;
		int64_t nparams = f->_nparameters + (f->_varparams?1:0);
		rabbit::ObjectPtr params = rabbit::Array::create(_get_shared_state(v),nparams);
	rabbit::ObjectPtr defparams = rabbit::Array::create(_get_shared_state(v),f->_ndefaultparams);
		for(int64_t n = 0; n<f->_nparameters; n++) {
			_array(params)->set((int64_t)n,f->_parameters[n]);
		}
	for(int64_t j = 0; j<f->_ndefaultparams; j++) {
			_array(defparams)->set((int64_t)j,_closure(o)->_defaultparams[j]);
		}
		if(f->_varparams) {
			_array(params)->set(nparams-1,SQString::create(_get_shared_state(v),_SC("..."),-1));
		}
		res->newSlot(SQString::create(_get_shared_state(v),_SC("native"),-1),false);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("name"),-1),f->_name);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("src"),-1),f->_sourcename);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("parameters"),-1),params);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("varargs"),-1),f->_varparams);
	res->newSlot(SQString::create(_get_shared_state(v),_SC("defparams"),-1),defparams);
	}
	else { //rabbit::OT_NATIVECLOSURE
		SQNativeClosure *nc = _nativeclosure(o);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("native"),-1),true);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("name"),-1),nc->_name);
		res->newSlot(SQString::create(_get_shared_state(v),_SC("paramscheck"),-1),nc->_nparamscheck);
		rabbit::ObjectPtr typecheck;
		if(nc->_typecheck.size() > 0) {
			typecheck =
				rabbit::Array::create(_get_shared_state(v), nc->_typecheck.size());
			for(uint64_t n = 0; n<nc->_typecheck.size(); n++) {
					_array(typecheck)->set((int64_t)n,nc->_typecheck[n]);
			}
		}
		res->newSlot(SQString::create(_get_shared_state(v),_SC("typecheck"),-1),typecheck);
	}
	v->push(res);
	return 1;
}



const rabbit::RegFunction SQSharedState::_closure_default_delegate_funcz[]={
	{_SC("call"),closure_call,-1, _SC("c")},
	{_SC("pcall"),closure_pcall,-1, _SC("c")},
	{_SC("acall"),closure_acall,2, _SC("ca")},
	{_SC("pacall"),closure_pacall,2, _SC("ca")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("bindenv"),closure_bindenv,2, _SC("c x|y|t")},
	{_SC("getinfos"),closure_getinfos,1, _SC("c")},
	{_SC("getroot"),closure_getroot,1, _SC("c")},
	{_SC("setroot"),closure_setroot,2, _SC("ct")},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//GENERATOR DEFAULT DELEGATE
static int64_t generator_getstatus(rabbit::VirtualMachine* v)
{
	rabbit::Object &o=stack_get(v,1);
	switch(_generator(o)->_state){
		case SQGenerator::eSuspended:v->push(SQString::create(_get_shared_state(v),_SC("suspended")));break;
		case SQGenerator::eRunning:v->push(SQString::create(_get_shared_state(v),_SC("running")));break;
		case SQGenerator::eDead:v->push(SQString::create(_get_shared_state(v),_SC("dead")));break;
	}
	return 1;
}

const rabbit::RegFunction SQSharedState::_generator_default_delegate_funcz[]={
	{_SC("getstatus"),generator_getstatus,1, _SC("g")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{NULL,(SQFUNCTION)0,0,NULL}
};

//THREAD DEFAULT DELEGATE
static int64_t thread_call(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		int64_t nparams = sq_gettop(v);
		_thread(o)->push(_thread(o)->_roottable);
		for(int64_t i = 2; i<(nparams+1); i++)
			sq_move(_thread(o),v,i);
		if(SQ_SUCCEEDED(sq_call(_thread(o),nparams,SQTrue,SQTrue))) {
			sq_move(v,_thread(o),-1);
			sq_pop(_thread(o),1);
			return 1;
		}
		v->_lasterror = _thread(o)->_lasterror;
		return SQ_ERROR;
	}
	return sq_throwerror(v,_SC("wrong parameter"));
}

static int64_t thread_wakeup(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		rabbit::VirtualMachine *thread = _thread(o);
		int64_t state = sq_getvmstate(thread);
		if(state != SQ_VMSTATE_SUSPENDED) {
			switch(state) {
				case SQ_VMSTATE_IDLE:
					return sq_throwerror(v,_SC("cannot wakeup a idle thread"));
				break;
				case SQ_VMSTATE_RUNNING:
					return sq_throwerror(v,_SC("cannot wakeup a running thread"));
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
	return sq_throwerror(v,_SC("wrong parameter"));
}

static int64_t thread_wakeupthrow(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		rabbit::VirtualMachine *thread = _thread(o);
		int64_t state = sq_getvmstate(thread);
		if(state != SQ_VMSTATE_SUSPENDED) {
			switch(state) {
				case SQ_VMSTATE_IDLE:
					return sq_throwerror(v,_SC("cannot wakeup a idle thread"));
				break;
				case SQ_VMSTATE_RUNNING:
					return sq_throwerror(v,_SC("cannot wakeup a running thread"));
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
	return sq_throwerror(v,_SC("wrong parameter"));
}

static int64_t thread_getstatus(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr &o = stack_get(v,1);
	switch(sq_getvmstate(_thread(o))) {
		case SQ_VMSTATE_IDLE:
			sq_pushstring(v,_SC("idle"),-1);
		break;
		case SQ_VMSTATE_RUNNING:
			sq_pushstring(v,_SC("running"),-1);
		break;
		case SQ_VMSTATE_SUSPENDED:
			sq_pushstring(v,_SC("suspended"),-1);
		break;
		default:
			return sq_throwerror(v,_SC("internal VM error"));
	}
	return 1;
}

static int64_t thread_getstackinfos(rabbit::VirtualMachine* v)
{
	rabbit::ObjectPtr o = stack_get(v,1);
	if(sq_type(o) == rabbit::OT_THREAD) {
		rabbit::VirtualMachine *thread = _thread(o);
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
				sq_throwerror(v,_SC("unknown error"));
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
	return sq_throwerror(v,_SC("wrong parameter"));
}

const rabbit::RegFunction SQSharedState::_thread_default_delegate_funcz[] = {
	{_SC("call"), thread_call, -1, _SC("v")},
	{_SC("wakeup"), thread_wakeup, -1, _SC("v")},
	{_SC("wakeupthrow"), thread_wakeupthrow, -2, _SC("v.b")},
	{_SC("getstatus"), thread_getstatus, 1, _SC("v")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("getstackinfos"),thread_getstackinfos,2, _SC("vn")},
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
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

const rabbit::RegFunction SQSharedState::_class_default_delegate_funcz[] = {
	{_SC("getattributes"), class_getattributes, 2, _SC("y.")},
	{_SC("setattributes"), class_setattributes, 3, _SC("y..")},
	{_SC("rawget"),container_rawget,2, _SC("y")},
	{_SC("rawset"),container_rawset,3, _SC("y")},
	{_SC("rawin"),container_rawexists,2, _SC("y")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{_SC("instance"),class_instance,1, _SC("y")},
	{_SC("getbase"),class_getbase,1, _SC("y")},
	{_SC("newmember"),class_newmember,-3, _SC("y")},
	{_SC("rawnewmember"),class_rawnewmember,-3, _SC("y")},
	{NULL,(SQFUNCTION)0,0,NULL}
};


static int64_t instance_getclass(rabbit::VirtualMachine* v)
{
	if(SQ_SUCCEEDED(sq_getclass(v,1)))
		return 1;
	return SQ_ERROR;
}

const rabbit::RegFunction SQSharedState::_instance_default_delegate_funcz[] = {
	{_SC("getclass"), instance_getclass, 1, _SC("x")},
	{_SC("rawget"),container_rawget,2, _SC("x")},
	{_SC("rawset"),container_rawset,3, _SC("x")},
	{_SC("rawin"),container_rawexists,2, _SC("x")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{NULL,(SQFUNCTION)0,0,NULL}
};

static int64_t weakref_ref(rabbit::VirtualMachine* v)
{
	if(SQ_FAILED(sq_getweakrefval(v,1)))
		return SQ_ERROR;
	return 1;
}

const rabbit::RegFunction SQSharedState::_weakref_default_delegate_funcz[] = {
	{_SC("ref"),weakref_ref,1, _SC("r")},
	{_SC("weakref"),obj_delegate_weakref,1, NULL },
	{_SC("tostring"),default_delegate_tostring,1, _SC(".")},
	{NULL,(SQFUNCTION)0,0,NULL}
};
