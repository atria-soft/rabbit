/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Compiler.hpp>
#include <rabbit/FuncState.hpp>

#ifndef NO_COMPILER
#include <rabbit/Compiler.hpp>

#include <rabbit/FunctionProto.hpp>
#include <rabbit/Instruction.hpp>
#include <rabbit/String.hpp>
#include <rabbit/Class.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/SharedState.hpp>



#include <rabbit/sqopcodes.hpp>
#include <rabbit/squtils.hpp>

#define UINT_MINUS_ONE (0xFFFFFFFFFFFFFFFF)


#ifdef _DEBUG_DUMP
rabbit::InstructionDesc g_InstrDesc[]={
	{"_OP_LINE"},
	{"_OP_LOAD"},
	{"_OP_LOADINT"},
	{"_OP_LOADFLOAT"},
	{"_OP_DLOAD"},
	{"_OP_TAILCALL"},
	{"_OP_CALL"},
	{"_OP_PREPCALL"},
	{"_OP_PREPCALLK"},
	{"_OP_GETK"},
	{"_OP_MOVE"},
	{"_OP_NEWSLOT"},
	{"_OP_DELETE"},
	{"_OP_SET"},
	{"_OP_GET"},
	{"_OP_EQ"},
	{"_OP_NE"},
	{"_OP_ADD"},
	{"_OP_SUB"},
	{"_OP_MUL"},
	{"_OP_DIV"},
	{"_OP_MOD"},
	{"_OP_BITW"},
	{"_OP_RETURN"},
	{"_OP_LOADNULLS"},
	{"_OP_LOADROOT"},
	{"_OP_LOADBOOL"},
	{"_OP_DMOVE"},
	{"_OP_JMP"},
	{"_OP_JCMP"},
	{"_OP_JZ"},
	{"_OP_SETOUTER"},
	{"_OP_GETOUTER"},
	{"_OP_NEWOBJ"},
	{"_OP_APPENDARRAY"},
	{"_OP_COMPARITH"},
	{"_OP_INC"},
	{"_OP_INCL"},
	{"_OP_PINC"},
	{"_OP_PINCL"},
	{"_OP_CMP"},
	{"_OP_EXISTS"},
	{"_OP_INSTANCEOF"},
	{"_OP_AND"},
	{"_OP_OR"},
	{"_OP_NEG"},
	{"_OP_NOT"},
	{"_OP_BWNOT"},
	{"_OP_CLOSURE"},
	{"_OP_YIELD"},
	{"_OP_RESUME"},
	{"_OP_FOREACH"},
	{"_OP_POSTFOREACH"},
	{"_OP_CLONE"},
	{"_OP_TYPEOF"},
	{"_OP_PUSHTRAP"},
	{"_OP_POPTRAP"},
	{"_OP_THROW"},
	{"_OP_NEWSLOTA"},
	{"_OP_GETBASE"},
	{"_OP_CLOSE"},
};
#endif


void rabbit::FuncState::addInstruction(SQOpcode _op,int64_t arg0,int64_t arg1,int64_t arg2,int64_t arg3){
	rabbit::Instruction i(_op,arg0,arg1,arg2,arg3);
	addInstruction(i);
}

static void dumpLiteral(rabbit::ObjectPtr &o) {
	switch(o.getType()){
		case rabbit::OT_STRING:
			printf("\"%s\"",o.getStringValue());
			break;
		case rabbit::OT_FLOAT:
			printf("{%f}",o.toFloat());
			break;
		case rabbit::OT_INTEGER:
			printf("{" _PRINT_INT_FMT "}",o.toInteger());
			break;
		case rabbit::OT_BOOL:
			printf("%s",o.toInteger()?"true":"false");
			break;
		default:
			printf("(%s %p)",getTypeName(o),(void*)o.toRaw());
			break;
	}
}

rabbit::FuncState::FuncState(rabbit::SharedState *ss,rabbit::FuncState *parent,compilererrorFunc efunc,void *ed)
{
		_nliterals = 0;
		_literals = rabbit::Table::create(ss,0);
		_strings =  rabbit::Table::create(ss,0);
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

void rabbit::FuncState::error(const char *err)
{
	_errfunc(_errtarget,err);
}

#ifdef _DEBUG_DUMP
void rabbit::FuncState::dump(rabbit::FunctionProto *func)
{
	uint64_t n=0,i;
	int64_t si;
	printf("rabbit::Instruction sizeof %d\n",(int32_t)sizeof(rabbit::Instruction));
	printf("rabbit::Object sizeof %d\n", (int32_t)sizeof(rabbit::Object));
	printf("--------------------------------------------------------------------\n");
	printf("*****FUNCTION [%s]\n",func->_name.isString() == true?func->_name.getStringValue():"unknown");
	printf("-----LITERALS\n");
	rabbit::ObjectPtr refidx,key,val;
	int64_t idx;
	etk::Vector<rabbit::ObjectPtr> templiterals;
	templiterals.resize(_nliterals);
	while((idx=_literals.toTable()->next(false,refidx,key,val))!=-1) {
		refidx=idx;
		templiterals[val.toInteger()]=key;
	}
	for(i=0;i<templiterals.size();i++){
		printf("[%d] ", (int32_t)n);
		dumpLiteral(templiterals[i]);
		printf("\n");
		n++;
	}
	printf("-----PARAMS\n");
	if(_varparams)
		printf("<<VARPARAMS>>\n");
	n=0;
	for(i=0;i<_parameters.size();i++){
		printf("[%d] ", (int32_t)n);
		dumpLiteral(_parameters[i]);
		printf("\n");
		n++;
	}
	printf("-----LOCALS\n");
	for(si=0;si<func->_nlocalvarinfos;si++){
		rabbit::LocalVarInfo lvi=func->_localvarinfos[si];
		printf("[%d] %s \t%d %d\n", (int32_t)lvi._pos,lvi._name.getStringValue(), (int32_t)lvi._start_op, (int32_t)lvi._end_op);
		n++;
	}
	printf("-----LINE INFO\n");
	for(i=0;i<_lineinfos.size();i++){
		rabbit::LineInfo li=_lineinfos[i];
		printf("op [%d] line [%d] \n", (int32_t)li._op, (int32_t)li._line);
		n++;
	}
	printf("-----dump\n");
	n=0;
	for(i=0;i<_instructions.size();i++){
		rabbit::Instruction &inst=_instructions[i];
		if(inst.op==_OP_LOAD || inst.op==_OP_DLOAD || inst.op==_OP_PREPCALLK || inst.op==_OP_GETK ){

			int64_t lidx = inst._arg1;
			printf("[%03d] %15s %d ", (int32_t)n,g_InstrDesc[inst.op].name,inst._arg0);
			if(lidx >= 0xFFFFFFFF)
				printf("null");
			else {
				int64_t refidx;
				rabbit::ObjectPtr val,key,refo;
				while(((refidx=_literals.toTable()->next(false,refo,key,val))!= -1) && (val.toInteger() != lidx)) {
					refo = refidx;
				}
				dumpLiteral(key);
			}
			if(inst.op != _OP_DLOAD) {
				printf(" %d %d \n",inst._arg2,inst._arg3);
			}
			else {
				printf(" %d ",inst._arg2);
				lidx = inst._arg3;
				if(lidx >= 0xFFFFFFFF)
					printf("null");
				else {
					int64_t refidx;
					rabbit::ObjectPtr val,key,refo;
					while(((refidx=_literals.toTable()->next(false,refo,key,val))!= -1) && (val.toInteger() != lidx)) {
						refo = refidx;
				}
				dumpLiteral(key);
				printf("\n");
			}
			}
		}
		else if(inst.op==_OP_LOADFLOAT) {
			printf("[%03d] %15s %d %f %d %d\n", (int32_t)n,g_InstrDesc[inst.op].name,inst._arg0,*((float_t*)&inst._arg1),inst._arg2,inst._arg3);
		}
	/*  else if(inst.op==_OP_ARITH){
			printf("[%03d] %15s %d %d %d %c\n",n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
		}*/
		else {
			printf("[%03d] %15s %d %d %d %d\n", (int32_t)n,g_InstrDesc[inst.op].name,inst._arg0,inst._arg1,inst._arg2,inst._arg3);
		}
		n++;
	}
	printf("-----\n");
	printf("stack size[%d]\n", (int32_t)func->_stacksize);
	printf("--------------------------------------------------------------------\n\n");
}
#endif

int64_t rabbit::FuncState::getNumericConstant(const int64_t cons)
{
	return getConstant(rabbit::ObjectPtr(cons));
}

int64_t rabbit::FuncState::getNumericConstant(const float_t cons)
{
	return getConstant(rabbit::ObjectPtr(cons));
}

int64_t rabbit::FuncState::getConstant(const rabbit::Object &cons)
{
	rabbit::ObjectPtr val;
	if(!_literals.toTable()->get(cons,val))
	{
		val = _nliterals;
		_literals.toTable()->newSlot(cons,val);
		_nliterals++;
		if(_nliterals > MAX_LITERALS) {
			val.Null();
			error("internal compiler error: too many literals");
		}
	}
	return val.toInteger();
}

void rabbit::FuncState::setIntructionParams(int64_t pos,int64_t arg0,int64_t arg1,int64_t arg2,int64_t arg3)
{
	_instructions[pos]._arg0=(unsigned char)*((uint64_t *)&arg0);
	_instructions[pos]._arg1=(int32_t)*((uint64_t *)&arg1);
	_instructions[pos]._arg2=(unsigned char)*((uint64_t *)&arg2);
	_instructions[pos]._arg3=(unsigned char)*((uint64_t *)&arg3);
}

void rabbit::FuncState::setIntructionParam(int64_t pos,int64_t arg,int64_t val)
{
	switch(arg){
		case 0:_instructions[pos]._arg0=(unsigned char)*((uint64_t *)&val);break;
		case 1:case 4:_instructions[pos]._arg1=(int32_t)*((uint64_t *)&val);break;
		case 2:_instructions[pos]._arg2=(unsigned char)*((uint64_t *)&val);break;
		case 3:_instructions[pos]._arg3=(unsigned char)*((uint64_t *)&val);break;
	};
}

int64_t rabbit::FuncState::allocStackPos()
{
	int64_t npos=_vlocals.size();
	_vlocals.pushBack(rabbit::LocalVarInfo());
	if(_vlocals.size()>((uint64_t)_stacksize)) {
		if(_stacksize>MAX_FUNC_STACKSIZE) error("internal compiler error: too many locals");
		_stacksize=_vlocals.size();
	}
	return npos;
}

int64_t rabbit::FuncState::pushTarget(int64_t n)
{
	if(n!=-1){
		_targetstack.pushBack(n);
		return n;
	}
	n=allocStackPos();
	_targetstack.pushBack(n);
	return n;
}

int64_t rabbit::FuncState::getUpTarget(int64_t n){
	return _targetstack[((_targetstack.size()-1)-n)];
}

int64_t rabbit::FuncState::topTarget(){
	return _targetstack.back();
}
int64_t rabbit::FuncState::popTarget()
{
	uint64_t npos=_targetstack.back();
	assert(npos < _vlocals.size());
	rabbit::LocalVarInfo &t = _vlocals[npos];
	if(t._name.isNull()==true){
		_vlocals.popBack();
	}
	_targetstack.popBack();
	return npos;
}

int64_t rabbit::FuncState::getStacksize()
{
	return _vlocals.size();
}

int64_t rabbit::FuncState::CountOuters(int64_t stacksize)
{
	int64_t outers = 0;
	int64_t k = _vlocals.size() - 1;
	while(k >= stacksize) {
		rabbit::LocalVarInfo &lvi = _vlocals[k];
		k--;
		if(lvi._end_op == UINT_MINUS_ONE) { //this means is an outer
			outers++;
		}
	}
	return outers;
}

void rabbit::FuncState::setStacksize(int64_t n)
{
	int64_t size=_vlocals.size();
	while(size>n){
		size--;
		rabbit::LocalVarInfo lvi = _vlocals.back();
		if(lvi._name.isNull()==false){
			if(lvi._end_op == UINT_MINUS_ONE) { //this means is an outer
				_outers--;
			}
			lvi._end_op = getCurrentPos();
			_localvarinfos.pushBack(lvi);
		}
		_vlocals.popBack();
	}
}

bool rabbit::FuncState::isConstant(const rabbit::Object &name,rabbit::Object &e)
{
	rabbit::ObjectPtr val;
	if(_sharedstate->_consts.toTable()->get(name,val)) {
		e = val;
		return true;
	}
	return false;
}

bool rabbit::FuncState::isLocal(uint64_t stkpos)
{
	if(stkpos>=_vlocals.size()) {
		return false;
	} else if(_vlocals[stkpos]._name.isNull()==false) {
		return true;
	}
	return false;
}

int64_t rabbit::FuncState::pushLocalVariable(const rabbit::Object &name)
{
	int64_t pos=_vlocals.size();
	rabbit::LocalVarInfo lvi;
	lvi._name=name;
	lvi._start_op=getCurrentPos()+1;
	lvi._pos=_vlocals.size();
	_vlocals.pushBack(lvi);
	if(_vlocals.size()>((uint64_t)_stacksize))_stacksize=_vlocals.size();
	return pos;
}



int64_t rabbit::FuncState::getLocalVariable(const rabbit::Object &name)
{
	int64_t locals=_vlocals.size();
	while(locals>=1){
		rabbit::LocalVarInfo &lvi = _vlocals[locals-1];
		if(    lvi._name.isString() == true
		    && lvi._name.toString() == name.toString()){
			return locals-1;
		}
		locals--;
	}
	return -1;
}

void rabbit::FuncState::markLocalAsOuter(int64_t pos)
{
	rabbit::LocalVarInfo &lvi = _vlocals[pos];
	lvi._end_op = UINT_MINUS_ONE;
	_outers++;
}

int64_t rabbit::FuncState::getOuterVariable(const rabbit::Object &name)
{
	int64_t outers = _outervalues.size();
	for(int64_t i = 0; i<outers; i++) {
		if(_outervalues[i]._name.toString() == name.toString())
			return i;
	}
	int64_t pos=-1;
	if(_parent) {
		pos = _parent->getLocalVariable(name);
		if(pos == -1) {
			pos = _parent->getOuterVariable(name);
			if(pos != -1) {
				_outervalues.pushBack(rabbit::OuterVar(name,rabbit::ObjectPtr(int64_t(pos)),otOUTER)); //local
				return _outervalues.size() - 1;
			}
		} else {
			_parent->markLocalAsOuter(pos);
			_outervalues.pushBack(rabbit::OuterVar(name,rabbit::ObjectPtr(int64_t(pos)),otLOCAL)); //local
			return _outervalues.size() - 1;
		}
	}
	return -1;
}

void rabbit::FuncState::addParameter(const rabbit::Object &name)
{
	pushLocalVariable(name);
	_parameters.pushBack(name);
}

void rabbit::FuncState::addLineInfos(int64_t line,bool lineop,bool force)
{
	if(_lastline!=line || force){
		rabbit::LineInfo li;
		li._line=line;li._op=(getCurrentPos()+1);
		if(lineop)addInstruction(_OP_LINE,0,line);
		if(_lastline!=line) {
			_lineinfos.pushBack(li);
		}
		_lastline=line;
	}
}

void rabbit::FuncState::discardTarget()
{
	int64_t discardedtarget = popTarget();
	int64_t size = _instructions.size();
	if(size > 0 && _optimization){
		rabbit::Instruction &pi = _instructions[size-1];//previous instruction
		switch(pi.op) {
		case _OP_SET:case _OP_NEWSLOT:case _OP_SETOUTER:case _OP_CALL:
			if(pi._arg0 == discardedtarget) {
				pi._arg0 = 0xFF;
			}
		}
	}
}

void rabbit::FuncState::addInstruction(rabbit::Instruction &i)
{
	int64_t size = _instructions.size();
	if(size > 0 && _optimization){ //simple optimizer
		rabbit::Instruction &pi = _instructions[size-1];//previous instruction
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

rabbit::Object rabbit::FuncState::createString(const char *s,int64_t len)
{
	rabbit::ObjectPtr ns(rabbit::String::create(_sharedstate,s,len));
	_strings.toTable()->newSlot(ns,(int64_t)1);
	return ns;
}

rabbit::Object rabbit::FuncState::createTable()
{
	rabbit::ObjectPtr nt(rabbit::Table::create(_sharedstate,0));
	_strings.toTable()->newSlot(nt,(int64_t)1);
	return nt;
}

rabbit::FunctionProto* rabbit::FuncState::buildProto() {
	rabbit::FunctionProto *f=rabbit::FunctionProto::create(_ss,_instructions.size(),
		_nliterals,_parameters.size(),_functions.size(),_outervalues.size(),
		_lineinfos.size(),_localvarinfos.size(),_defaultparams.size());

	rabbit::ObjectPtr refidx,key,val;
	int64_t idx;

	f->_stacksize = _stacksize;
	f->_sourcename = _sourcename;
	f->_bgenerator = _bgenerator;
	f->_name = _name;

	while((idx=_literals.toTable()->next(false,refidx,key,val))!=-1) {
		f->_literals[val.toInteger()]=key;
		refidx=idx;
	}

	for(uint64_t nf = 0; nf < _functions.size(); nf++) f->_functions[nf] = _functions[nf];
	for(uint64_t np = 0; np < _parameters.size(); np++) f->_parameters[np] = _parameters[np];
	for(uint64_t no = 0; no < _outervalues.size(); no++) f->_outervalues[no] = _outervalues[no];
	for(uint64_t nl = 0; nl < _localvarinfos.size(); nl++) f->_localvarinfos[nl] = _localvarinfos[nl];
	for(uint64_t ni = 0; ni < _lineinfos.size(); ni++) f->_lineinfos[ni] = _lineinfos[ni];
	for(uint64_t nd = 0; nd < _defaultparams.size(); nd++) f->_defaultparams[nd] = _defaultparams[nd];

	memcpy(f->_instructions,&_instructions[0],_instructions.size()*sizeof(rabbit::Instruction));

	f->_varparams = _varparams;

	return f;
}

rabbit::FuncState *rabbit::FuncState::pushChildState(rabbit::SharedState *ss) {
	FuncState *child = ETK_NEW(rabbit::FuncState, ss, this, _errfunc, _errtarget);
	_childstates.pushBack(child);
	return child;
}

void rabbit::FuncState::popChildState() {
	FuncState *child = _childstates.back();
	ETK_DELETE(FuncState, child);
	_childstates.popBack();
}

rabbit::FuncState::~FuncState() {
	while(_childstates.size() > 0) {
		popChildState();
	}
}

#endif
