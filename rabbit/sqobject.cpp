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
#include <rabbit/Array.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/UserData.hpp>
#include <rabbit/sqfuncproto.hpp>

#include <rabbit/sqclosure.hpp>

SQString *SQString::create(SQSharedState *ss,const rabbit::Char *s,int64_t len)
{
	SQString *str=ADD_STRING(ss,s,len);
	return str;
}

void SQString::release()
{
	REMOVE_STRING(_sharedstate,this);
}

int64_t SQString::next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval)
{
	int64_t idx = (int64_t)translateIndex(refpos);
	while(idx < _len){
		outkey = (int64_t)idx;
		outval = (int64_t)((uint64_t)_val[idx]);
		//return idx for the next iteration
		return ++idx;
	}
	//nothing to iterate anymore
	return -1;
}


bool SQGenerator::yield(rabbit::VirtualMachine *v,int64_t target)
{
	if(_state==eSuspended) { v->raise_error(_SC("internal vm error, yielding dead generator"));  return false;}
	if(_state==eDead) { v->raise_error(_SC("internal vm error, yielding a dead generator")); return false; }
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

bool SQGenerator::resume(rabbit::VirtualMachine *v,rabbit::ObjectPtr &dest)
{
	if(_state==eDead){ v->raise_error(_SC("resuming dead generator")); return false; }
	if(_state==eRunning){ v->raise_error(_SC("resuming active generator")); return false; }
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
		v->callDebugHook(_SC('c'));

	return true;
}

void rabbit::Array::extend(const rabbit::Array *a){
	int64_t xlen;
	if((xlen=a->size()))
		for(int64_t i=0;i<xlen;i++)
			append((*a)[i]);
}

const rabbit::Char* SQFunctionProto::getLocal(rabbit::VirtualMachine *vm,uint64_t stackbase,uint64_t nseq,uint64_t nop)
{
	uint64_t nvars=_nlocalvarinfos;
	const rabbit::Char *res=NULL;
	if(nvars>=nseq){
		for(uint64_t i=0;i<nvars;i++){
			if(_localvarinfos[i]._start_op<=nop && _localvarinfos[i]._end_op>=nop)
			{
				if(nseq==0){
					vm->push(vm->_stack[stackbase+_localvarinfos[i]._pos]);
					res=_stringval(_localvarinfos[i]._name);
					break;
				}
				nseq--;
			}
		}
	}
	return res;
}


int64_t SQFunctionProto::getLine(SQInstruction *curr)
{
	int64_t op = (int64_t)(curr-_instructions);
	int64_t line=_lineinfos[0]._line;
	int64_t low = 0;
	int64_t high = _nlineinfos - 1;
	int64_t mid = 0;
	while(low <= high)
	{
		mid = low + ((high - low) >> 1);
		int64_t curop = _lineinfos[mid]._op;
		if(curop > op)
		{
			high = mid - 1;
		}
		else if(curop < op) {
			if(mid < (_nlineinfos - 1)
				&& _lineinfos[mid + 1]._op >= op) {
				break;
			}
			low = mid + 1;
		}
		else { //equal
			break;
		}
	}

	while(mid > 0 && _lineinfos[mid]._op >= op) mid--;

	line = _lineinfos[mid]._line;

	return line;
}

SQClosure::~SQClosure()
{
	__Objrelease(_root);
	__Objrelease(_env);
	__Objrelease(_base);
}

#define _CHECK_IO(exp)  { if(!exp)return false; }
bool SafeWrite(rabbit::VirtualMachine* v,SQWRITEFUNC write,rabbit::UserPointer up,rabbit::UserPointer dest,int64_t size)
{
	if(write(up,dest,size) != size) {
		v->raise_error(_SC("io error (write function failure)"));
		return false;
	}
	return true;
}

bool SafeRead(rabbit::VirtualMachine* v,SQWRITEFUNC read,rabbit::UserPointer up,rabbit::UserPointer dest,int64_t size)
{
	if(size && read(up,dest,size) != size) {
		v->raise_error(_SC("io error, read function failure, the origin stream could be corrupted/trucated"));
		return false;
	}
	return true;
}

bool WriteTag(rabbit::VirtualMachine* v,SQWRITEFUNC write,rabbit::UserPointer up,uint32_t tag)
{
	return SafeWrite(v,write,up,&tag,sizeof(tag));
}

bool CheckTag(rabbit::VirtualMachine* v,SQWRITEFUNC read,rabbit::UserPointer up,uint32_t tag)
{
	uint32_t t;
	_CHECK_IO(SafeRead(v,read,up,&t,sizeof(t)));
	if(t != tag){
		v->raise_error(_SC("invalid or corrupted closure stream"));
		return false;
	}
	return true;
}

bool WriteObject(rabbit::VirtualMachine* v,rabbit::UserPointer up,SQWRITEFUNC write,rabbit::ObjectPtr &o)
{
	uint32_t _type = (uint32_t)sq_type(o);
	_CHECK_IO(SafeWrite(v,write,up,&_type,sizeof(_type)));
	switch(sq_type(o)){
	case rabbit::OT_STRING:
		_CHECK_IO(SafeWrite(v,write,up,&_string(o)->_len,sizeof(int64_t)));
		_CHECK_IO(SafeWrite(v,write,up,_stringval(o),sq_rsl(_string(o)->_len)));
		break;
	case rabbit::OT_BOOL:
	case rabbit::OT_INTEGER:
		_CHECK_IO(SafeWrite(v,write,up,&_integer(o),sizeof(int64_t)));break;
	case rabbit::OT_FLOAT:
		_CHECK_IO(SafeWrite(v,write,up,&_float(o),sizeof(float_t)));break;
	case rabbit::OT_NULL:
		break;
	default:
		v->raise_error(_SC("cannot serialize a %s"),getTypeName(o));
		return false;
	}
	return true;
}

bool ReadObject(rabbit::VirtualMachine* v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &o)
{
	uint32_t _type;
	_CHECK_IO(SafeRead(v,read,up,&_type,sizeof(_type)));
	rabbit::ObjectType t = (rabbit::ObjectType)_type;
	switch(t){
	case rabbit::OT_STRING:{
		int64_t len;
		_CHECK_IO(SafeRead(v,read,up,&len,sizeof(int64_t)));
		_CHECK_IO(SafeRead(v,read,up,_get_shared_state(v)->getScratchPad(sq_rsl(len)),sq_rsl(len)));
		o=SQString::create(_get_shared_state(v),_get_shared_state(v)->getScratchPad(-1),len);
				   }
		break;
	case rabbit::OT_INTEGER:{
		int64_t i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(int64_t))); o = i; break;
					}
	case rabbit::OT_BOOL:{
		int64_t i;
		_CHECK_IO(SafeRead(v,read,up,&i,sizeof(int64_t))); o._type = rabbit::OT_BOOL; o._unVal.nInteger = i; break;
					}
	case rabbit::OT_FLOAT:{
		float_t f;
		_CHECK_IO(SafeRead(v,read,up,&f,sizeof(float_t))); o = f; break;
				  }
	case rabbit::OT_NULL:
		o.Null();
		break;
	default:
		v->raise_error(_SC("cannot serialize a %s"),IdType2Name(t));
		return false;
	}
	return true;
}

bool SQClosure::save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write)
{
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_HEAD));
	_CHECK_IO(WriteTag(v,write,up,sizeof(rabbit::Char)));
	_CHECK_IO(WriteTag(v,write,up,sizeof(int64_t)));
	_CHECK_IO(WriteTag(v,write,up,sizeof(float_t)));
	_CHECK_IO(_function->save(v,up,write));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_TAIL));
	return true;
}

bool SQClosure::load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret)
{
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_HEAD));
	_CHECK_IO(CheckTag(v,read,up,sizeof(rabbit::Char)));
	_CHECK_IO(CheckTag(v,read,up,sizeof(int64_t)));
	_CHECK_IO(CheckTag(v,read,up,sizeof(float_t)));
	rabbit::ObjectPtr func;
	_CHECK_IO(SQFunctionProto::load(v,up,read,func));
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_TAIL));
	ret = SQClosure::create(_get_shared_state(v),_funcproto(func),_table(v->_roottable)->getWeakRef(rabbit::OT_TABLE));
	//FIXME: load an root for this closure
	return true;
}

SQFunctionProto::SQFunctionProto(SQSharedState *ss)
{
	_stacksize=0;
	_bgenerator=false;
}

SQFunctionProto::~SQFunctionProto()
{
}

bool SQFunctionProto::save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write)
{
	int64_t i,nliterals = _nliterals,nparameters = _nparameters;
	int64_t noutervalues = _noutervalues,nlocalvarinfos = _nlocalvarinfos;
	int64_t nlineinfos=_nlineinfos,ninstructions = _ninstructions,nfunctions=_nfunctions;
	int64_t ndefaultparams = _ndefaultparams;
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(WriteObject(v,up,write,_sourcename));
	_CHECK_IO(WriteObject(v,up,write,_name));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,&nliterals,sizeof(nliterals)));
	_CHECK_IO(SafeWrite(v,write,up,&nparameters,sizeof(nparameters)));
	_CHECK_IO(SafeWrite(v,write,up,&noutervalues,sizeof(noutervalues)));
	_CHECK_IO(SafeWrite(v,write,up,&nlocalvarinfos,sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeWrite(v,write,up,&nlineinfos,sizeof(nlineinfos)));
	_CHECK_IO(SafeWrite(v,write,up,&ndefaultparams,sizeof(ndefaultparams)));
	_CHECK_IO(SafeWrite(v,write,up,&ninstructions,sizeof(ninstructions)));
	_CHECK_IO(SafeWrite(v,write,up,&nfunctions,sizeof(nfunctions)));
	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nliterals;i++){
		_CHECK_IO(WriteObject(v,up,write,_literals[i]));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nparameters;i++){
		_CHECK_IO(WriteObject(v,up,write,_parameters[i]));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<noutervalues;i++){
		_CHECK_IO(SafeWrite(v,write,up,&_outervalues[i]._type,sizeof(uint64_t)));
		_CHECK_IO(WriteObject(v,up,write,_outervalues[i]._src));
		_CHECK_IO(WriteObject(v,up,write,_outervalues[i]._name));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nlocalvarinfos;i++){
		SQLocalVarInfo &lvi=_localvarinfos[i];
		_CHECK_IO(WriteObject(v,up,write,lvi._name));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._pos,sizeof(uint64_t)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._start_op,sizeof(uint64_t)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._end_op,sizeof(uint64_t)));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_lineinfos,sizeof(SQLineInfo)*nlineinfos));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_defaultparams,sizeof(int64_t)*ndefaultparams));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_instructions,sizeof(SQInstruction)*ninstructions));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nfunctions;i++){
		_CHECK_IO(_funcproto(_functions[i])->save(v,up,write));
	}
	_CHECK_IO(SafeWrite(v,write,up,&_stacksize,sizeof(_stacksize)));
	_CHECK_IO(SafeWrite(v,write,up,&_bgenerator,sizeof(_bgenerator)));
	_CHECK_IO(SafeWrite(v,write,up,&_varparams,sizeof(_varparams)));
	return true;
}

bool SQFunctionProto::load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret)
{
	int64_t i, nliterals,nparameters;
	int64_t noutervalues ,nlocalvarinfos ;
	int64_t nlineinfos,ninstructions ,nfunctions,ndefaultparams ;
	rabbit::ObjectPtr sourcename, name;
	rabbit::ObjectPtr o;
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(ReadObject(v, up, read, sourcename));
	_CHECK_IO(ReadObject(v, up, read, name));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, &nliterals, sizeof(nliterals)));
	_CHECK_IO(SafeRead(v,read,up, &nparameters, sizeof(nparameters)));
	_CHECK_IO(SafeRead(v,read,up, &noutervalues, sizeof(noutervalues)));
	_CHECK_IO(SafeRead(v,read,up, &nlocalvarinfos, sizeof(nlocalvarinfos)));
	_CHECK_IO(SafeRead(v,read,up, &nlineinfos, sizeof(nlineinfos)));
	_CHECK_IO(SafeRead(v,read,up, &ndefaultparams, sizeof(ndefaultparams)));
	_CHECK_IO(SafeRead(v,read,up, &ninstructions, sizeof(ninstructions)));
	_CHECK_IO(SafeRead(v,read,up, &nfunctions, sizeof(nfunctions)));


	SQFunctionProto *f = SQFunctionProto::create(NULL,ninstructions,nliterals,nparameters,
			nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams);
	rabbit::ObjectPtr proto = f; //gets a ref in case of failure
	f->_sourcename = sourcename;
	f->_name = name;

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0;i < nliterals; i++){
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_literals[i] = o;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < nparameters; i++){
		_CHECK_IO(ReadObject(v, up, read, o));
		f->_parameters[i] = o;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < noutervalues; i++){
		uint64_t type;
		rabbit::ObjectPtr name;
		_CHECK_IO(SafeRead(v,read,up, &type, sizeof(uint64_t)));
		_CHECK_IO(ReadObject(v, up, read, o));
		_CHECK_IO(ReadObject(v, up, read, name));
		f->_outervalues[i] = SQOuterVar(name,o, (SQOuterType)type);
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < nlocalvarinfos; i++){
		SQLocalVarInfo lvi;
		_CHECK_IO(ReadObject(v, up, read, lvi._name));
		_CHECK_IO(SafeRead(v,read,up, &lvi._pos, sizeof(uint64_t)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._start_op, sizeof(uint64_t)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._end_op, sizeof(uint64_t)));
		f->_localvarinfos[i] = lvi;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_lineinfos, sizeof(SQLineInfo)*nlineinfos));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_defaultparams, sizeof(int64_t)*ndefaultparams));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_instructions, sizeof(SQInstruction)*ninstructions));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	for(i = 0; i < nfunctions; i++){
		_CHECK_IO(_funcproto(o)->load(v, up, read, o));
		f->_functions[i] = o;
	}
	_CHECK_IO(SafeRead(v,read,up, &f->_stacksize, sizeof(f->_stacksize)));
	_CHECK_IO(SafeRead(v,read,up, &f->_bgenerator, sizeof(f->_bgenerator)));
	_CHECK_IO(SafeRead(v,read,up, &f->_varparams, sizeof(f->_varparams)));

	ret = f;
	return true;
}

