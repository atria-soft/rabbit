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
	SQFuncState(SQSharedState *ss,SQFuncState *parent,compilererrorFunc efunc,void *ed);
	~SQFuncState();
#ifdef _DEBUG_DUMP
	void dump(SQFunctionProto *func);
#endif
	void error(const SQChar *err);
	SQFuncState *pushChildState(SQSharedState *ss);
	void popChildState();
	void addInstruction(SQOpcode _op,int64_t arg0=0,int64_t arg1=0,int64_t arg2=0,int64_t arg3=0){SQInstruction i(_op,arg0,arg1,arg2,arg3);addInstruction(i);}
	void addInstruction(SQInstruction &i);
	void setIntructionParams(int64_t pos,int64_t arg0,int64_t arg1,int64_t arg2=0,int64_t arg3=0);
	void setIntructionParam(int64_t pos,int64_t arg,int64_t val);
	SQInstruction &getInstruction(int64_t pos){return _instructions[pos];}
	void popInstructions(int64_t size){for(int64_t i=0;i<size;i++)_instructions.pop_back();}
	void setStacksize(int64_t n);
	int64_t CountOuters(int64_t stacksize);
	void snoozeOpt(){_optimization=false;}
	void addDefaultParam(int64_t trg) { _defaultparams.push_back(trg); }
	int64_t getDefaultParamCount() { return _defaultparams.size(); }
	int64_t getCurrentPos(){return _instructions.size()-1;}
	int64_t getNumericConstant(const int64_t cons);
	int64_t getNumericConstant(const float_t cons);
	int64_t pushLocalVariable(const SQObject &name);
	void addParameter(const SQObject &name);
	//void addOuterValue(const SQObject &name);
	int64_t getLocalVariable(const SQObject &name);
	void markLocalAsOuter(int64_t pos);
	int64_t getOuterVariable(const SQObject &name);
	int64_t generateCode();
	int64_t getStacksize();
	int64_t calcStackFramesize();
	void addLineInfos(int64_t line,bool lineop,bool force=false);
	SQFunctionProto *buildProto();
	int64_t allocStackPos();
	int64_t pushTarget(int64_t n=-1);
	int64_t popTarget();
	int64_t topTarget();
	int64_t getUpTarget(int64_t n);
	void discardTarget();
	bool isLocal(uint64_t stkpos);
	SQObject createString(const SQChar *s,int64_t len = -1);
	SQObject createTable();
	bool isConstant(const SQObject &name,SQObject &e);
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
	int64_t getConstant(const SQObject &cons);
private:
	compilererrorFunc _errfunc;
	void *_errtarget;
	SQSharedState *_ss;
};


