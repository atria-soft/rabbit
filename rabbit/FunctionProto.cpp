/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/FunctionProto.hpp>
#include <rabbit/squtils.hpp>
#include <etk/types.hpp>
#include <etk/Allocator.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/Closure.hpp>

#include <rabbit/String.hpp>
#include <rabbit/VirtualMachine.hpp>





rabbit::FunctionProto* rabbit::FunctionProto::create(rabbit::SharedState *ss,int64_t ninstructions,
	int64_t nliterals,int64_t nparameters,
	int64_t nfunctions,int64_t noutervalues,
	int64_t nlineinfos,int64_t nlocalvarinfos,int64_t ndefaultparams)
{
	rabbit::FunctionProto *f;
	//I compact the whole class and members in a single memory allocation
	f = (rabbit::FunctionProto *)sq_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
	new ((char*)f) rabbit::FunctionProto(ss);
	f->_ninstructions = ninstructions;
	f->_literals = (rabbit::ObjectPtr*)&f->_instructions[ninstructions];
	f->_nliterals = nliterals;
	f->_parameters = (rabbit::ObjectPtr*)&f->_literals[nliterals];
	f->_nparameters = nparameters;
	f->_functions = (rabbit::ObjectPtr*)&f->_parameters[nparameters];
	f->_nfunctions = nfunctions;
	f->_outervalues = (rabbit::OuterVar*)&f->_functions[nfunctions];
	f->_noutervalues = noutervalues;
	f->_lineinfos = (rabbit::LineInfo *)&f->_outervalues[noutervalues];
	f->_nlineinfos = nlineinfos;
	f->_localvarinfos = (rabbit::LocalVarInfo *)&f->_lineinfos[nlineinfos];
	f->_nlocalvarinfos = nlocalvarinfos;
	f->_defaultparams = (int64_t *)&f->_localvarinfos[nlocalvarinfos];
	f->_ndefaultparams = ndefaultparams;

	_CONSTRUCT_VECTOR(ObjectPtr, f->_nliterals, f->_literals);
	_CONSTRUCT_VECTOR(ObjectPtr, f->_nparameters, f->_parameters);
	_CONSTRUCT_VECTOR(ObjectPtr, f->_nfunctions, f->_functions);
	_CONSTRUCT_VECTOR(OuterVar, f->_noutervalues, f->_outervalues);
	//_CONSTRUCT_VECTOR(LineInfo, f->_nlineinfos,f->_lineinfos); //not required are 2 integers
	_CONSTRUCT_VECTOR(LocalVarInfo, f->_nlocalvarinfos, f->_localvarinfos);
	return f;
}
void rabbit::FunctionProto::release(){
	_DESTRUCT_VECTOR(ObjectPtr,_nliterals,_literals);
	_DESTRUCT_VECTOR(ObjectPtr,_nparameters,_parameters);
	_DESTRUCT_VECTOR(ObjectPtr,_nfunctions,_functions);
	_DESTRUCT_VECTOR(OuterVar,_noutervalues,_outervalues);
	//_DESTRUCT_VECTOR(rabbit::LineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
	_DESTRUCT_VECTOR(LocalVarInfo,_nlocalvarinfos,_localvarinfos);
	int64_t size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
	this->~FunctionProto();
	sq_vm_free(this,size);
}

const char* rabbit::FunctionProto::getLocal(rabbit::VirtualMachine *vm,uint64_t stackbase,uint64_t nseq,uint64_t nop)
{
	uint64_t nvars=_nlocalvarinfos;
	const char *res=NULL;
	if(nvars>=nseq){
		for(uint64_t i=0;i<nvars;i++){
			if(_localvarinfos[i]._start_op<=nop && _localvarinfos[i]._end_op>=nop)
			{
				if(nseq==0){
					vm->push(vm->_stack[stackbase+_localvarinfos[i]._pos]);
					res=_localvarinfos[i]._name.getStringValue();
					break;
				}
				nseq--;
			}
		}
	}
	return res;
}


int64_t rabbit::FunctionProto::getLine(rabbit::Instruction *curr)
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



rabbit::FunctionProto::FunctionProto(rabbit::SharedState *ss)
{
	_stacksize=0;
	_bgenerator=false;
}

rabbit::FunctionProto::~FunctionProto()
{
}
