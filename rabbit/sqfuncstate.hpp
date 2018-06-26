/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/squtils.hpp>

struct SQFuncState
{
	SQFuncState(SQSharedState *ss,SQFuncState *parent,CompilerErrorFunc efunc,void *ed);
	~SQFuncState();
#ifdef _DEBUG_DUMP
	void Dump(SQFunctionProto *func);
#endif
	void Error(const SQChar *err);
	SQFuncState *PushChildState(SQSharedState *ss);
	void PopChildState();
	void AddInstruction(SQOpcode _op,int64_t arg0=0,int64_t arg1=0,int64_t arg2=0,int64_t arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);AddInstruction(i);}
	void AddInstruction(SQInstruction &i);
	void SetIntructionParams(int64_t pos,int64_t arg0,int64_t arg1,int64_t arg2=0,int64_t arg3=0);
	void SetIntructionParam(int64_t pos,int64_t arg,int64_t val);
	SQInstruction &GetInstruction(int64_t pos){return _instructions[pos];}
	void PopInstructions(int64_t size){for(int64_t i=0;i<size;i++)_instructions.pop_back();}
	void SetStackSize(int64_t n);
	int64_t CountOuters(int64_t stacksize);
	void SnoozeOpt(){_optimization=false;}
	void AddDefaultParam(int64_t trg) { _defaultparams.push_back(trg); }
	int64_t GetDefaultParamCount() { return _defaultparams.size(); }
	int64_t GetCurrentPos(){return _instructions.size()-1;}
	int64_t GetNumericConstant(const int64_t cons);
	int64_t GetNumericConstant(const float_t cons);
	int64_t PushLocalVariable(const SQObject &name);
	void AddParameter(const SQObject &name);
	//void AddOuterValue(const SQObject &name);
	int64_t GetLocalVariable(const SQObject &name);
	void MarkLocalAsOuter(int64_t pos);
	int64_t GetOuterVariable(const SQObject &name);
	int64_t GenerateCode();
	int64_t GetStackSize();
	int64_t CalcStackFrameSize();
	void AddLineInfos(int64_t line,bool lineop,bool force=false);
	SQFunctionProto *BuildProto();
	int64_t AllocStackPos();
	int64_t PushTarget(int64_t n=-1);
	int64_t PopTarget();
	int64_t TopTarget();
	int64_t GetUpTarget(int64_t n);
	void DiscardTarget();
	bool IsLocal(uint64_t stkpos);
	SQObject CreateString(const SQChar *s,int64_t len = -1);
	SQObject CreateTable();
	bool IsConstant(const SQObject &name,SQObject &e);
	int64_t _returnexp;
	SQLocalVarInfoVec _vlocals;
	SQIntVec _targetstack;
	int64_t _stacksize;
	bool _varparams;
	bool _bgenerator;
	SQIntVec _unresolvedbreaks;
	SQIntVec _unresolvedcontinues;
	SQObjectPtrVec _functions;
	SQObjectPtrVec _parameters;
	SQOuterVarVec _outervalues;
	SQInstructionVec _instructions;
	SQLocalVarInfoVec _localvarinfos;
	SQObjectPtr _literals;
	SQObjectPtr _strings;
	SQObjectPtr _name;
	SQObjectPtr _sourcename;
	int64_t _nliterals;
	SQLineInfoVec _lineinfos;
	SQFuncState *_parent;
	SQIntVec _scope_blocks;
	SQIntVec _breaktargets;
	SQIntVec _continuetargets;
	SQIntVec _defaultparams;
	int64_t _lastline;
	int64_t _traps; //contains number of nested exception traps
	int64_t _outers;
	bool _optimization;
	SQSharedState *_sharedstate;
	sqvector<SQFuncState*> _childstates;
	int64_t GetConstant(const SQObject &cons);
private:
	CompilerErrorFunc _errfunc;
	void *_errtarget;
	SQSharedState *_ss;
};


