/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/sqopcodes.hpp>

enum SQOuterType {
	otLOCAL = 0,
	otOUTER = 1
};

struct SQOuterVar
{

	SQOuterVar(){}
	SQOuterVar(const rabbit::ObjectPtr &name,const rabbit::ObjectPtr &src,SQOuterType t)
	{
		_name = name;
		_src=src;
		_type=t;
	}
	SQOuterVar(const SQOuterVar &ov)
	{
		_type=ov._type;
		_src=ov._src;
		_name=ov._name;
	}
	SQOuterType _type;
	rabbit::ObjectPtr _name;
	rabbit::ObjectPtr _src;
};

struct SQLocalVarInfo
{
	SQLocalVarInfo():_start_op(0),_end_op(0),_pos(0){}
	SQLocalVarInfo(const SQLocalVarInfo &lvi)
	{
		_name=lvi._name;
		_start_op=lvi._start_op;
		_end_op=lvi._end_op;
		_pos=lvi._pos;
	}
	rabbit::ObjectPtr _name;
	uint64_t _start_op;
	uint64_t _end_op;
	uint64_t _pos;
};

struct SQLineInfo { int64_t _line;int64_t _op; };

typedef etk::Vector<SQOuterVar> SQOuterVarVec;
typedef etk::Vector<SQLocalVarInfo> SQLocalVarInfoVec;
typedef etk::Vector<SQLineInfo> SQLineInfoVec;

#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(SQFunctionProto) \
		+((ni-1)*sizeof(SQInstruction))+(nl*sizeof(rabbit::ObjectPtr)) \
		+(nparams*sizeof(rabbit::ObjectPtr))+(nfuncs*sizeof(rabbit::ObjectPtr)) \
		+(nouters*sizeof(SQOuterVar))+(nlineinf*sizeof(SQLineInfo)) \
		+(localinf*sizeof(SQLocalVarInfo))+(defparams*sizeof(int64_t)))


struct SQFunctionProto : public rabbit::RefCounted
{
private:
	SQFunctionProto(SQSharedState *ss);
	~SQFunctionProto();

public:
	static SQFunctionProto *create(SQSharedState *ss,int64_t ninstructions,
		int64_t nliterals,int64_t nparameters,
		int64_t nfunctions,int64_t noutervalues,
		int64_t nlineinfos,int64_t nlocalvarinfos,int64_t ndefaultparams)
	{
		SQFunctionProto *f;
		//I compact the whole class and members in a single memory allocation
		f = (SQFunctionProto *)sq_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
		new (f) SQFunctionProto(ss);
		f->_ninstructions = ninstructions;
		f->_literals = (rabbit::ObjectPtr*)&f->_instructions[ninstructions];
		f->_nliterals = nliterals;
		f->_parameters = (rabbit::ObjectPtr*)&f->_literals[nliterals];
		f->_nparameters = nparameters;
		f->_functions = (rabbit::ObjectPtr*)&f->_parameters[nparameters];
		f->_nfunctions = nfunctions;
		f->_outervalues = (SQOuterVar*)&f->_functions[nfunctions];
		f->_noutervalues = noutervalues;
		f->_lineinfos = (SQLineInfo *)&f->_outervalues[noutervalues];
		f->_nlineinfos = nlineinfos;
		f->_localvarinfos = (SQLocalVarInfo *)&f->_lineinfos[nlineinfos];
		f->_nlocalvarinfos = nlocalvarinfos;
		f->_defaultparams = (int64_t *)&f->_localvarinfos[nlocalvarinfos];
		f->_ndefaultparams = ndefaultparams;

		_CONSTRUCT_VECTOR(rabbit::ObjectPtr,f->_nliterals,f->_literals);
		_CONSTRUCT_VECTOR(rabbit::ObjectPtr,f->_nparameters,f->_parameters);
		_CONSTRUCT_VECTOR(rabbit::ObjectPtr,f->_nfunctions,f->_functions);
		_CONSTRUCT_VECTOR(SQOuterVar,f->_noutervalues,f->_outervalues);
		//_CONSTRUCT_VECTOR(SQLineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
		_CONSTRUCT_VECTOR(SQLocalVarInfo,f->_nlocalvarinfos,f->_localvarinfos);
		return f;
	}
	void release(){
		_DESTRUCT_VECTOR(ObjectPtr,_nliterals,_literals);
		_DESTRUCT_VECTOR(ObjectPtr,_nparameters,_parameters);
		_DESTRUCT_VECTOR(ObjectPtr,_nfunctions,_functions);
		_DESTRUCT_VECTOR(SQOuterVar,_noutervalues,_outervalues);
		//_DESTRUCT_VECTOR(SQLineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
		_DESTRUCT_VECTOR(SQLocalVarInfo,_nlocalvarinfos,_localvarinfos);
		int64_t size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
		this->~SQFunctionProto();
		sq_vm_free(this,size);
	}

	const rabbit::Char* getLocal(rabbit::VirtualMachine *v,uint64_t stackbase,uint64_t nseq,uint64_t nop);
	int64_t getLine(SQInstruction *curr);
	bool save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write);
	static bool load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret);
	rabbit::ObjectPtr _sourcename;
	rabbit::ObjectPtr _name;
	int64_t _stacksize;
	bool _bgenerator;
	int64_t _varparams;

	int64_t _nlocalvarinfos;
	SQLocalVarInfo *_localvarinfos;

	int64_t _nlineinfos;
	SQLineInfo *_lineinfos;

	int64_t _nliterals;
	rabbit::ObjectPtr *_literals;

	int64_t _nparameters;
	rabbit::ObjectPtr *_parameters;

	int64_t _nfunctions;
	rabbit::ObjectPtr *_functions;

	int64_t _noutervalues;
	SQOuterVar *_outervalues;

	int64_t _ndefaultparams;
	int64_t *_defaultparams;

	int64_t _ninstructions;
	SQInstruction _instructions[1];
};
