/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/sqpcheader.hpp>
#ifndef NO_COMPILER
#include <rabbit/sqcompiler.hpp>
#include <rabbit/sqstring.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/sqopcodes.hpp>
#include <rabbit/sqfuncstate.hpp>

#ifdef _DEBUG_DUMP
SQInstructionDesc g_InstrDesc[]={
	{_SC("_OP_LINE")},
	{_SC("_OP_LOAD")},
	{_SC("_OP_LOADINT")},
	{_SC("_OP_LOADFLOAT")},
	{_SC("_OP_DLOAD")},
	{_SC("_OP_TAILCALL")},
	{_SC("_OP_CALL")},
	{_SC("_OP_PREPCALL")},
	{_SC("_OP_PREPCALLK")},
	{_SC("_OP_GETK")},
	{_SC("_OP_MOVE")},
	{_SC("_OP_NEWSLOT")},
	{_SC("_OP_DELETE")},
	{_SC("_OP_SET")},
	{_SC("_OP_GET")},
	{_SC("_OP_EQ")},
	{_SC("_OP_NE")},
	{_SC("_OP_ADD")},
	{_SC("_OP_SUB")},
	{_SC("_OP_MUL")},
	{_SC("_OP_DIV")},
	{_SC("_OP_MOD")},
	{_SC("_OP_BITW")},
	{_SC("_OP_RETURN")},
	{_SC("_OP_LOADNULLS")},
	{_SC("_OP_LOADROOT")},
	{_SC("_OP_LOADBOOL")},
	{_SC("_OP_DMOVE")},
	{_SC("_OP_JMP")},
	{_SC("_OP_JCMP")},
	{_SC("_OP_JZ")},
	{_SC("_OP_SETOUTER")},
	{_SC("_OP_GETOUTER")},
	{_SC("_OP_NEWOBJ")},
	{_SC("_OP_APPENDARRAY")},
	{_SC("_OP_COMPARITH")},
	{_SC("_OP_INC")},
	{_SC("_OP_INCL")},
	{_SC("_OP_PINC")},
	{_SC("_OP_PINCL")},
	{_SC("_OP_CMP")},
	{_SC("_OP_EXISTS")},
	{_SC("_OP_INSTANCEOF")},
	{_SC("_OP_AND")},
	{_SC("_OP_OR")},
	{_SC("_OP_NEG")},
	{_SC("_OP_NOT")},
	{_SC("_OP_BWNOT")},
	{_SC("_OP_CLOSURE")},
	{_SC("_OP_YIELD")},
	{_SC("_OP_RESUME")},
	{_SC("_OP_FOREACH")},
	{_SC("_OP_POSTFOREACH")},
	{_SC("_OP_CLONE")},
	{_SC("_OP_TYPEOF")},
	{_SC("_OP_PUSHTRAP")},
	{_SC("_OP_POPTRAP")},
	{_SC("_OP_THROW")},
	{_SC("_OP_NEWSLOTA")},
	{_SC("_OP_GETBASE")},
	{_SC("_OP_CLOSE")},
};
#endif
void dumpLiteral(SQObjectPtr &o)
{
	switch(sq_type(o)){
		case OT_STRING: scprintf(_SC("\"%s\""),_stringval(o));break;
		case OT_FLOAT: scprintf(_SC("{%f}"),_float(o));break;
		case OT_INTEGER: scprintf(_SC("{") _PRINT_INT_FMT _SC("}"),_integer(o));break;
		case OT_BOOL: scprintf(_SC("%s"),_integer(o)?_SC("true"):_SC("false"));break;
		default: scprintf(_SC("(%s %p)"),getTypeName(o),(void*)_rawval(o));break; break; //shut up compiler
	}
}

SQFuncState::SQFuncState(SQSharedState *ss,SQFuncState *parent,compilererrorFunc efunc,void *ed)
{
		_nliterals = 0;
		_literals = SQTable::create(ss,0);
		_strings =  SQTable::create(ss,0);
		_sharedstate = ss;
		_lastline = 0;
		_optimization = true;
		_parent = parent;
		_stacksize = 0;
		_traps = 0;
		_returnexp = 0;
		_varparams = false;
		_errfunc = efunc;
		_errtarget = ed;
		_bgenerator = false;
		_outers = 0;
		_ss = ss;

}

void SQFuncState::error(const SQChar *err)
{
	_errfunc(_errtarget,err);
}

#ifdef _DEBUG_DUMP
void SQFuncState::dump(SQFunctionProto *func)
{
	uint64_t n=0,i;
	int64_t si;
	scprintf(_SC("SQInstruction sizeof %d\n"),(int32_t)sizeof(SQInstruction));
	scprintf(_SC("SQObject sizeof %d\n"), (int32_t)sizeof(SQObject));
	scprintf(_SC("--------------------------------------------------------------------\n"));
	scprintf(_SC("*****FUNCTION [%s]\n"),sq_type(func->_name)==OT_STRING?_stringval(func->_name):_SC("unknown"));
	scprintf(_SC("-----LITERALS\n"));
	SQObjectPtr refidx,key,val;
	int64_t idx;
	SQObjectPtrVec templiterals;
	templiterals.resize(_nliterals);
	while((idx=_table(_literals)->next(false,refidx,key,val))!=-1) {
		refidx=idx;
		templiterals[_integer(val)]=key;
	}
	for(i=0;i<templiterals.size();i++){
		scprintf(_SC("[%d] "), (int32_t)n);
		dumpLiteral(templiterals[i]);
		scprintf(_SC("\n"));
		n++;
	}
	scprintf(_SC("-----PARAMS\n"));
	if(_varparams)
		scprintf(_SC("<<VARPARAMS>>\n"));
	n=0;
	for(i=0;i<_parameters.size();i++){
		scprintf(_SC("[%d] "), (int32_t)n);
		dumpLiteral(_parameters[i]);
		scprintf(_SC("\n"));
		n++;
	}
	scprintf(_SC("-----LOCALS\n"));
	for(si=0;si<func->_nlocalvarinfos;si++){
		SQLocalVarInfo lvi=func->_localvarinfos[si];
		scprintf(_SC("[%d] %s \t%d %d\n"), (int32_t)lvi._pos,_stringval(lvi._name), (int32_t)lvi._start_op, (int32_t)lvi._end_op);
		n++;
	}
	scprintf(_SC("-----LINE INFO\n"));
	for(i=0;i<_lineinfos.size();i++){
		SQLineInfo li=_lineinfos[i];
		scprintf(_SC("op [%d] line [%d] \n"), (int32_t)li._op, (int32_t)li._line);
		n++;
	}
	scprintf(_SC("-----dump\n"));
	n=0;
	for(i=0;i<_instructions.size();i++){
		SQInstruction &inst=_instructions[i];
		if(inst.op==_OP_LOAD || inst.op==_OP_DLOAD || inst.op==_OP_PREPCALLK || inst.op==_OP_GETK ){

			int64_t lidx = inst._arg1;
			scprintf(_SC("[%03d] %15s %d "), (int32_t)n,g_InstrDesc[inst.op].name,inst._arg0);
			if(lidx >= 0xFFFFFFFF)
				scprintf(_SC("null"));
			else {
				int64_t refidx;
				SQObjectPtr val,key,refo;
				while(((refidx=_table(_literals)->next(false,refo,key,val))!= -1) && (_integer(val) != lidx)) {
					refo = refidx;
				}
				dumpLiteral(key);
			}
			if(inst.op != _OP_DLOAD) {
				scprintf(_SC(" %d %d \n"),inst._arg2,inst._arg3);
			}
			else {
				scprintf(_SC(" %d "),inst._arg2);
				lidx = inst._arg3;
				if(lidx >= 0xFFFFFFFF)
					scprintf(_SC("null"));
				else {
					int64_t refidx;
					SQObjectPtr val,key,refo;
					while(((refidx=_table(_literals)->next(false,refo,key,val))!= -1) && (_integer(val) != lidx)) {
						refo = refidx;
				}
				dumpLiteral(key);
				scprintf(_SC("\n"));
			}
			}
		}
		else if(inst.op==_OP_LOADFLOAT) {
			scprintf(_SC("[%03d] %15s %d %f %d %d\n"), (int32_t)n,g_InstrDesc[inst.op].name,inst._arg0,*((float_t*)&inst._arg1),inst._arg2,inst._arg3);
		}
	/*  else if(inst.op==_OP_ARITH){
			scprintf(_SC("[%03d] %15s %d %d %d %c\n"),n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
		}*/
		else {
			scprintf(_SC("[%03d] %15s %d %d %d %d\n"), (int32_t)n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
		}
		n++;
	}
	scprintf(_SC("-----\n"));
	scprintf(_SC("stack size[%d]\n"), (int32_t)func->_stacksize);
	scprintf(_SC("--------------------------------------------------------------------\n\n"));
}
#endif

int64_t SQFuncState::getNumericConstant(const int64_t cons)
{
	return getConstant(SQObjectPtr(cons));
}

int64_t SQFuncState::getNumericConstant(const float_t cons)
{
	return getConstant(SQObjectPtr(cons));
}

int64_t SQFuncState::getConstant(const SQObject &cons)
{
	SQObjectPtr val;
	if(!_table(_literals)->get(cons,val))
	{
		val = _nliterals;
		_table(_literals)->newSlot(cons,val);
		_nliterals++;
		if(_nliterals > MAX_LITERALS) {
			val.Null();
			error(_SC("internal compiler error: too many literals"));
		}
	}
	return _integer(val);
}

void SQFuncState::setIntructionParams(int64_t pos,int64_t arg0,int64_t arg1,int64_t arg2,int64_t arg3)
{
	_instructions[pos]._arg0=(unsigned char)*((uint64_t *)&arg0);
	_instructions[pos]._arg1=(int32_t)*((uint64_t *)&arg1);
	_instructions[pos]._arg2=(unsigned char)*((uint64_t *)&arg2);
	_instructions[pos]._arg3=(unsigned char)*((uint64_t *)&arg3);
}

void SQFuncState::setIntructionParam(int64_t pos,int64_t arg,int64_t val)
{
	switch(arg){
		case 0:_instructions[pos]._arg0=(unsigned char)*((uint64_t *)&val);break;
		case 1:case 4:_instructions[pos]._arg1=(int32_t)*((uint64_t *)&val);break;
		case 2:_instructions[pos]._arg2=(unsigned char)*((uint64_t *)&val);break;
		case 3:_instructions[pos]._arg3=(unsigned char)*((uint64_t *)&val);break;
	};
}

int64_t SQFuncState::allocStackPos()
{
	int64_t npos=_vlocals.size();
	_vlocals.pushBack(SQLocalVarInfo());
	if(_vlocals.size()>((uint64_t)_stacksize)) {
		if(_stacksize>MAX_FUNC_STACKSIZE) error(_SC("internal compiler error: too many locals"));
		_stacksize=_vlocals.size();
	}
	return npos;
}

int64_t SQFuncState::pushTarget(int64_t n)
{
	if(n!=-1){
		_targetstack.pushBack(n);
		return n;
	}
	n=allocStackPos();
	_targetstack.pushBack(n);
	return n;
}

int64_t SQFuncState::getUpTarget(int64_t n){
	return _targetstack[((_targetstack.size()-1)-n)];
}

int64_t SQFuncState::topTarget(){
	return _targetstack.back();
}
int64_t SQFuncState::popTarget()
{
	uint64_t npos=_targetstack.back();
	assert(npos < _vlocals.size());
	SQLocalVarInfo &t = _vlocals[npos];
	if(sq_type(t._name)==OT_NULL){
		_vlocals.popBack();
	}
	_targetstack.popBack();
	return npos;
}

int64_t SQFuncState::getStacksize()
{
	return _vlocals.size();
}

int64_t SQFuncState::CountOuters(int64_t stacksize)
{
	int64_t outers = 0;
	int64_t k = _vlocals.size() - 1;
	while(k >= stacksize) {
		SQLocalVarInfo &lvi = _vlocals[k];
		k--;
		if(lvi._end_op == UINT_MINUS_ONE) { //this means is an outer
			outers++;
		}
	}
	return outers;
}

void SQFuncState::setStacksize(int64_t n)
{
	int64_t size=_vlocals.size();
	while(size>n){
		size--;
		SQLocalVarInfo lvi = _vlocals.back();
		if(sq_type(lvi._name)!=OT_NULL){
			if(lvi._end_op == UINT_MINUS_ONE) { //this means is an outer
				_outers--;
			}
			lvi._end_op = getCurrentPos();
			_localvarinfos.pushBack(lvi);
		}
		_vlocals.popBack();
	}
}

bool SQFuncState::isConstant(const SQObject &name,SQObject &e)
{
	SQObjectPtr val;
	if(_table(_sharedstate->_consts)->get(name,val)) {
		e = val;
		return true;
	}
	return false;
}

bool SQFuncState::isLocal(uint64_t stkpos)
{
	if(stkpos>=_vlocals.size())return false;
	else if(sq_type(_vlocals[stkpos]._name)!=OT_NULL)return true;
	return false;
}

int64_t SQFuncState::pushLocalVariable(const SQObject &name)
{
	int64_t pos=_vlocals.size();
	SQLocalVarInfo lvi;
	lvi._name=name;
	lvi._start_op=getCurrentPos()+1;
	lvi._pos=_vlocals.size();
	_vlocals.pushBack(lvi);
	if(_vlocals.size()>((uint64_t)_stacksize))_stacksize=_vlocals.size();
	return pos;
}



int64_t SQFuncState::getLocalVariable(const SQObject &name)
{
	int64_t locals=_vlocals.size();
	while(locals>=1){
		SQLocalVarInfo &lvi = _vlocals[locals-1];
		if(sq_type(lvi._name)==OT_STRING && _string(lvi._name)==_string(name)){
			return locals-1;
		}
		locals--;
	}
	return -1;
}

void SQFuncState::markLocalAsOuter(int64_t pos)
{
	SQLocalVarInfo &lvi = _vlocals[pos];
	lvi._end_op = UINT_MINUS_ONE;
	_outers++;
}

int64_t SQFuncState::getOuterVariable(const SQObject &name)
{
	int64_t outers = _outervalues.size();
	for(int64_t i = 0; i<outers; i++) {
		if(_string(_outervalues[i]._name) == _string(name))
			return i;
	}
	int64_t pos=-1;
	if(_parent) {
		pos = _parent->getLocalVariable(name);
		if(pos == -1) {
			pos = _parent->getOuterVariable(name);
			if(pos != -1) {
				_outervalues.pushBack(SQOuterVar(name,SQObjectPtr(int64_t(pos)),otOUTER)); //local
				return _outervalues.size() - 1;
			}
		}
		else {
			_parent->markLocalAsOuter(pos);
			_outervalues.pushBack(SQOuterVar(name,SQObjectPtr(int64_t(pos)),otLOCAL)); //local
			return _outervalues.size() - 1;


		}
	}
	return -1;
}

void SQFuncState::addParameter(const SQObject &name)
{
	pushLocalVariable(name);
	_parameters.pushBack(name);
}

void SQFuncState::addLineInfos(int64_t line,bool lineop,bool force)
{
	if(_lastline!=line || force){
		SQLineInfo li;
		li._line=line;li._op=(getCurrentPos()+1);
		if(lineop)addInstruction(_OP_LINE,0,line);
		if(_lastline!=line) {
			_lineinfos.pushBack(li);
		}
		_lastline=line;
	}
}

void SQFuncState::discardTarget()
{
	int64_t discardedtarget = popTarget();
	int64_t size = _instructions.size();
	if(size > 0 && _optimization){
		SQInstruction &pi = _instructions[size-1];//previous instruction
		switch(pi.op) {
		case _OP_SET:case _OP_NEWSLOT:case _OP_SETOUTER:case _OP_CALL:
			if(pi._arg0 == discardedtarget) {
				pi._arg0 = 0xFF;
			}
		}
	}
}

void SQFuncState::addInstruction(SQInstruction &i)
{
	int64_t size = _instructions.size();
	if(size > 0 && _optimization){ //simple optimizer
		SQInstruction &pi = _instructions[size-1];//previous instruction
		switch(i.op) {
		case _OP_JZ:
			if( pi.op == _OP_CMP && pi._arg1 < 0xFF) {
				pi.op = _OP_JCMP;
				pi._arg0 = (unsigned char)pi._arg1;
				pi._arg1 = i._arg1;
				return;
			}
			break;
		case _OP_SET:
		case _OP_NEWSLOT:
			if(i._arg0 == i._arg3) {
				i._arg0 = 0xFF;
			}
			break;
		case _OP_SETOUTER:
			if(i._arg0 == i._arg2) {
				i._arg0 = 0xFF;
			}
			break;
		case _OP_RETURN:
			if( _parent && i._arg0 != MAX_FUNC_STACKSIZE && pi.op == _OP_CALL && _returnexp < size-1) {
				pi.op = _OP_TAILCALL;
			} else if(pi.op == _OP_CLOSE){
				pi = i;
				return;
			}
		break;
		case _OP_GET:
			if( pi.op == _OP_LOAD && pi._arg0 == i._arg2 && (!isLocal(pi._arg0))){
				pi._arg1 = pi._arg1;
				pi._arg2 = (unsigned char)i._arg1;
				pi.op = _OP_GETK;
				pi._arg0 = i._arg0;

				return;
			}
		break;
		case _OP_PREPCALL:
			if( pi.op == _OP_LOAD  && pi._arg0 == i._arg1 && (!isLocal(pi._arg0))){
				pi.op = _OP_PREPCALLK;
				pi._arg0 = i._arg0;
				pi._arg1 = pi._arg1;
				pi._arg2 = i._arg2;
				pi._arg3 = i._arg3;
				return;
			}
			break;
		case _OP_APPENDARRAY: {
			int64_t aat = -1;
			switch(pi.op) {
			case _OP_LOAD: aat = AAT_LITERAL; break;
			case _OP_LOADINT: aat = AAT_INT; break;
			case _OP_LOADBOOL: aat = AAT_BOOL; break;
			case _OP_LOADFLOAT: aat = AAT_FLOAT; break;
			default: break;
			}
			if(aat != -1 && pi._arg0 == i._arg1 && (!isLocal(pi._arg0))){
				pi.op = _OP_APPENDARRAY;
				pi._arg0 = i._arg0;
				pi._arg1 = pi._arg1;
				pi._arg2 = (unsigned char)aat;
				pi._arg3 = MAX_FUNC_STACKSIZE;
				return;
			}
							  }
			break;
		case _OP_MOVE:
			switch(pi.op) {
			case _OP_GET: case _OP_ADD: case _OP_SUB: case _OP_MUL: case _OP_DIV: case _OP_MOD: case _OP_BITW:
			case _OP_LOADINT: case _OP_LOADFLOAT: case _OP_LOADBOOL: case _OP_LOAD:

				if(pi._arg0 == i._arg1)
				{
					pi._arg0 = i._arg0;
					_optimization = false;
					//_result_elimination = false;
					return;
				}
			}

			if(pi.op == _OP_MOVE)
			{
				pi.op = _OP_DMOVE;
				pi._arg2 = i._arg0;
				pi._arg3 = (unsigned char)i._arg1;
				return;
			}
			break;
		case _OP_LOAD:
			if(pi.op == _OP_LOAD && i._arg1 < 256) {
				pi.op = _OP_DLOAD;
				pi._arg2 = i._arg0;
				pi._arg3 = (unsigned char)i._arg1;
				return;
			}
			break;
		case _OP_EQ:case _OP_NE:
			if(pi.op == _OP_LOAD && pi._arg0 == i._arg1 && (!isLocal(pi._arg0) ))
			{
				pi.op = i.op;
				pi._arg0 = i._arg0;
				pi._arg1 = pi._arg1;
				pi._arg2 = i._arg2;
				pi._arg3 = MAX_FUNC_STACKSIZE;
				return;
			}
			break;
		case _OP_LOADNULLS:
			if((pi.op == _OP_LOADNULLS && pi._arg0+pi._arg1 == i._arg0)) {

				pi._arg1 = pi._arg1 + 1;
				pi.op = _OP_LOADNULLS;
				return;
			}
			break;
		case _OP_LINE:
			if(pi.op == _OP_LINE) {
				_instructions.popBack();
				_lineinfos.popBack();
			}
			break;
		}
	}
	_optimization = true;
	_instructions.pushBack(i);
}

SQObject SQFuncState::createString(const SQChar *s,int64_t len)
{
	SQObjectPtr ns(SQString::create(_sharedstate,s,len));
	_table(_strings)->newSlot(ns,(int64_t)1);
	return ns;
}

SQObject SQFuncState::createTable()
{
	SQObjectPtr nt(SQTable::create(_sharedstate,0));
	_table(_strings)->newSlot(nt,(int64_t)1);
	return nt;
}

SQFunctionProto *SQFuncState::buildProto()
{

	SQFunctionProto *f=SQFunctionProto::create(_ss,_instructions.size(),
		_nliterals,_parameters.size(),_functions.size(),_outervalues.size(),
		_lineinfos.size(),_localvarinfos.size(),_defaultparams.size());

	SQObjectPtr refidx,key,val;
	int64_t idx;

	f->_stacksize = _stacksize;
	f->_sourcename = _sourcename;
	f->_bgenerator = _bgenerator;
	f->_name = _name;

	while((idx=_table(_literals)->next(false,refidx,key,val))!=-1) {
		f->_literals[_integer(val)]=key;
		refidx=idx;
	}

	for(uint64_t nf = 0; nf < _functions.size(); nf++) f->_functions[nf] = _functions[nf];
	for(uint64_t np = 0; np < _parameters.size(); np++) f->_parameters[np] = _parameters[np];
	for(uint64_t no = 0; no < _outervalues.size(); no++) f->_outervalues[no] = _outervalues[no];
	for(uint64_t nl = 0; nl < _localvarinfos.size(); nl++) f->_localvarinfos[nl] = _localvarinfos[nl];
	for(uint64_t ni = 0; ni < _lineinfos.size(); ni++) f->_lineinfos[ni] = _lineinfos[ni];
	for(uint64_t nd = 0; nd < _defaultparams.size(); nd++) f->_defaultparams[nd] = _defaultparams[nd];

	memcpy(f->_instructions,&_instructions[0],_instructions.size()*sizeof(SQInstruction));

	f->_varparams = _varparams;

	return f;
}

SQFuncState *SQFuncState::pushChildState(SQSharedState *ss)
{
	SQFuncState *child = (SQFuncState *)sq_malloc(sizeof(SQFuncState));
	new (child) SQFuncState(ss,this,_errfunc,_errtarget);
	_childstates.pushBack(child);
	return child;
}

void SQFuncState::popChildState()
{
	SQFuncState *child = _childstates.back();
	sq_delete(child,SQFuncState);
	_childstates.popBack();
}

SQFuncState::~SQFuncState()
{
	while(_childstates.size() > 0)
	{
		popChildState();
	}
}

#endif
