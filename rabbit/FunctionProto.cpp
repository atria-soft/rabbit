/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/FunctionProto.hpp>


const rabbit::Char* rabbit::FunctionProto::getLocal(rabbit::VirtualMachine *vm,uint64_t stackbase,uint64_t nseq,uint64_t nop)
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



bool rabbit::FunctionProto::save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write)
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
		rabbit::LocalVarInfo &lvi=_localvarinfos[i];
		_CHECK_IO(WriteObject(v,up,write,lvi._name));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._pos,sizeof(uint64_t)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._start_op,sizeof(uint64_t)));
		_CHECK_IO(SafeWrite(v,write,up,&lvi._end_op,sizeof(uint64_t)));
	}

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_lineinfos,sizeof(rabbit::LineInfo)*nlineinfos));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_defaultparams,sizeof(int64_t)*ndefaultparams));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeWrite(v,write,up,_instructions,sizeof(rabbit::Instruction)*ninstructions));

	_CHECK_IO(WriteTag(v,write,up,SQ_CLOSURESTREAM_PART));
	for(i=0;i<nfunctions;i++){
		_CHECK_IO(_funcproto(_functions[i])->save(v,up,write));
	}
	_CHECK_IO(SafeWrite(v,write,up,&_stacksize,sizeof(_stacksize)));
	_CHECK_IO(SafeWrite(v,write,up,&_bgenerator,sizeof(_bgenerator)));
	_CHECK_IO(SafeWrite(v,write,up,&_varparams,sizeof(_varparams)));
	return true;
}

bool rabbit::FunctionProto::load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret)
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


	rabbit::FunctionProto *f = rabbit::FunctionProto::create(NULL,ninstructions,nliterals,nparameters,
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
		f->_outervalues[i] = rabbit::OuterVar(name,o, (rabbit::OuterType)type);
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));

	for(i = 0; i < nlocalvarinfos; i++){
		rabbit::LocalVarInfo lvi;
		_CHECK_IO(ReadObject(v, up, read, lvi._name));
		_CHECK_IO(SafeRead(v,read,up, &lvi._pos, sizeof(uint64_t)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._start_op, sizeof(uint64_t)));
		_CHECK_IO(SafeRead(v,read,up, &lvi._end_op, sizeof(uint64_t)));
		f->_localvarinfos[i] = lvi;
	}
	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_lineinfos, sizeof(rabbit::LineInfo)*nlineinfos));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_defaultparams, sizeof(int64_t)*ndefaultparams));

	_CHECK_IO(CheckTag(v,read,up,SQ_CLOSURESTREAM_PART));
	_CHECK_IO(SafeRead(v,read,up, f->_instructions, sizeof(rabbit::Instruction)*ninstructions));

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

