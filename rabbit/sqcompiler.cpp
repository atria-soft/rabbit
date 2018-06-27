/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/sqpcheader.hpp>
#ifndef NO_COMPILER
#include <stdarg.h>
#include <setjmp.h>
#include <rabbit/sqopcodes.hpp>
#include <rabbit/sqstring.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqcompiler.hpp>
#include <rabbit/sqfuncstate.hpp>
#include <rabbit/sqlexer.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/sqtable.hpp>

#define EXPR   1
#define OBJECT 2
#define BASE   3
#define LOCAL  4
#define OUTER  5

struct SQExpState {
  int64_t  etype;	   /* expr. type; one of EXPR, OBJECT, BASE, OUTER or LOCAL */
  int64_t  epos;		/* expr. location on stack; -1 for OBJECT and BASE */
  bool	   donot_get;   /* signal not to deref the next value */
};

#define MAX_COMPILER_ERROR_LEN 256

struct SQScope {
	int64_t outers;
	int64_t stacksize;
};

#define BEGIN_SCOPE() SQScope __oldscope__ = _scope; \
					 _scope.outers = _fs->_outers; \
					 _scope.stacksize = _fs->getStacksize();

#define RESOLVE_OUTERS() if(_fs->getStacksize() != _scope.stacksize) { \
							if(_fs->CountOuters(_scope.stacksize)) { \
								_fs->addInstruction(_OP_CLOSE,0,_scope.stacksize); \
							} \
						}

#define END_SCOPE_NO_CLOSE() {  if(_fs->getStacksize() != _scope.stacksize) { \
							_fs->setStacksize(_scope.stacksize); \
						} \
						_scope = __oldscope__; \
					}

#define END_SCOPE() {   int64_t oldouters = _fs->_outers;\
						if(_fs->getStacksize() != _scope.stacksize) { \
							_fs->setStacksize(_scope.stacksize); \
							if(oldouters != _fs->_outers) { \
								_fs->addInstruction(_OP_CLOSE,0,_scope.stacksize); \
							} \
						} \
						_scope = __oldscope__; \
					}

#define BEGIN_BREAKBLE_BLOCK()  int64_t __nbreaks__=_fs->_unresolvedbreaks.size(); \
							int64_t __ncontinues__=_fs->_unresolvedcontinues.size(); \
							_fs->_breaktargets.pushBack(0);_fs->_continuetargets.pushBack(0);

#define END_BREAKBLE_BLOCK(continue_target) {__nbreaks__=_fs->_unresolvedbreaks.size()-__nbreaks__; \
					__ncontinues__=_fs->_unresolvedcontinues.size()-__ncontinues__; \
					if(__ncontinues__>0)ResolveContinues(_fs,__ncontinues__,continue_target); \
					if(__nbreaks__>0)ResolveBreaks(_fs,__nbreaks__); \
					_fs->_breaktargets.popBack();_fs->_continuetargets.popBack();}

class SQcompiler
{
public:
	SQcompiler(rabbit::VirtualMachine *v, SQLEXREADFUNC rg, rabbit::UserPointer up, const rabbit::Char* sourcename, bool raiseerror, bool lineinfo)
	{
		_vm=v;
		_lex.init(_get_shared_state(v), rg, up,Throwerror,this);
		_sourcename = SQString::create(_get_shared_state(v), sourcename);
		_lineinfo = lineinfo;_raiseerror = raiseerror;
		_scope.outers = 0;
		_scope.stacksize = 0;
		_compilererror[0] = _SC('\0');
	}
	static void Throwerror(void *ud, const rabbit::Char *s) {
		SQcompiler *c = (SQcompiler *)ud;
		c->error(s);
	}
	void error(const rabbit::Char *s, ...)
	{
		va_list vl;
		va_start(vl, s);
		scvsprintf(_compilererror, MAX_COMPILER_ERROR_LEN, s, vl);
		va_end(vl);
		longjmp(_errorjmp,1);
	}
	void Lex(){ _token = _lex.Lex();}
	rabbit::Object Expect(int64_t tok)
	{

		if(_token != tok) {
			if(_token == TK_CONSTRUCTOR && tok == TK_IDENTIFIER) {
				//do nothing
			}
			else {
				const rabbit::Char *etypename;
				if(tok > 255) {
					switch(tok)
					{
					case TK_IDENTIFIER:
						etypename = _SC("IDENTIFIER");
						break;
					case TK_STRING_LITERAL:
						etypename = _SC("STRING_LITERAL");
						break;
					case TK_INTEGER:
						etypename = _SC("INTEGER");
						break;
					case TK_FLOAT:
						etypename = _SC("FLOAT");
						break;
					default:
						etypename = _lex.tok2Str(tok);
					}
					error(_SC("expected '%s'"), etypename);
				}
				error(_SC("expected '%c'"), tok);
			}
		}
		rabbit::ObjectPtr ret;
		switch(tok)
		{
		case TK_IDENTIFIER:
			ret = _fs->createString(_lex._svalue);
			break;
		case TK_STRING_LITERAL:
			ret = _fs->createString(_lex._svalue,_lex._longstr.size()-1);
			break;
		case TK_INTEGER:
			ret = rabbit::ObjectPtr(_lex._nvalue);
			break;
		case TK_FLOAT:
			ret = rabbit::ObjectPtr(_lex._fvalue);
			break;
		}
		Lex();
		return ret;
	}
	bool IsEndOfStatement() { return ((_lex._prevtoken == _SC('\n')) || (_token == RABBIT_EOB) || (_token == _SC('}')) || (_token == _SC(';'))); }
	void OptionalSemicolon()
	{
		if(_token == _SC(';')) { Lex(); return; }
		if(!IsEndOfStatement()) {
			error(_SC("end of statement expected (; or lf)"));
		}
	}
	void MoveIfCurrentTargetisLocal() {
		int64_t trg = _fs->topTarget();
		if(_fs->isLocal(trg)) {
			trg = _fs->popTarget(); //pops the target and moves it
			_fs->addInstruction(_OP_MOVE, _fs->pushTarget(), trg);
		}
	}
	bool compile(rabbit::ObjectPtr &o)
	{
		_debugline = 1;
		_debugop = 0;

		SQFuncState funcstate(_get_shared_state(_vm), NULL,Throwerror,this);
		funcstate._name = SQString::create(_get_shared_state(_vm), _SC("main"));
		_fs = &funcstate;
		_fs->addParameter(_fs->createString(_SC("this")));
		_fs->addParameter(_fs->createString(_SC("vargv")));
		_fs->_varparams = true;
		_fs->_sourcename = _sourcename;
		int64_t stacksize = _fs->getStacksize();
		if(setjmp(_errorjmp) == 0) {
			Lex();
			while(_token > 0){
				Statement();
				if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
			}
			_fs->setStacksize(stacksize);
			_fs->addLineInfos(_lex._currentline, _lineinfo, true);
			_fs->addInstruction(_OP_RETURN, 0xFF);
			_fs->setStacksize(0);
			o =_fs->buildProto();
#ifdef _DEBUG_DUMP
			_fs->dump(_funcproto(o));
#endif
		}
		else {
			if(_raiseerror && _get_shared_state(_vm)->_compilererrorhandler) {
				_get_shared_state(_vm)->_compilererrorhandler(_vm, _compilererror, sq_type(_sourcename) == rabbit::OT_STRING?_stringval(_sourcename):_SC("unknown"),
					_lex._currentline, _lex._currentcolumn);
			}
			_vm->_lasterror = SQString::create(_get_shared_state(_vm), _compilererror, -1);
			return false;
		}
		return true;
	}
	void Statements()
	{
		while(_token != _SC('}') && _token != TK_DEFAULT && _token != TK_CASE) {
			Statement();
			if(_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
		}
	}
	void Statement(bool closeframe = true)
	{
		_fs->addLineInfos(_lex._currentline, _lineinfo);
		switch(_token){
		case _SC(';'):  Lex();				  break;
		case TK_IF:	 IfStatement();		  break;
		case TK_WHILE:	  WhileStatement();	   break;
		case TK_DO:	 DoWhileStatement();	 break;
		case TK_FOR:		ForStatement();		 break;
		case TK_FOREACH:	ForEachStatement();	 break;
		case TK_SWITCH: SwitchStatement();	  break;
		case TK_LOCAL:	  LocalDeclStatement();   break;
		case TK_RETURN:
		case TK_YIELD: {
			SQOpcode op;
			if(_token == TK_RETURN) {
				op = _OP_RETURN;
			}
			else {
				op = _OP_YIELD;
				_fs->_bgenerator = true;
			}
			Lex();
			if(!IsEndOfStatement()) {
				int64_t retexp = _fs->getCurrentPos()+1;
				CommaExpr();
				if(op == _OP_RETURN && _fs->_traps > 0)
					_fs->addInstruction(_OP_POPTRAP, _fs->_traps, 0);
				_fs->_returnexp = retexp;
				_fs->addInstruction(op, 1, _fs->popTarget(),_fs->getStacksize());
			}
			else{
				if(op == _OP_RETURN && _fs->_traps > 0)
					_fs->addInstruction(_OP_POPTRAP, _fs->_traps ,0);
				_fs->_returnexp = -1;
				_fs->addInstruction(op, 0xFF,0,_fs->getStacksize());
			}
			break;}
		case TK_BREAK:
			if(_fs->_breaktargets.size() <= 0)error(_SC("'break' has to be in a loop block"));
			if(_fs->_breaktargets.back() > 0){
				_fs->addInstruction(_OP_POPTRAP, _fs->_breaktargets.back(), 0);
			}
			RESOLVE_OUTERS();
			_fs->addInstruction(_OP_JMP, 0, -1234);
			_fs->_unresolvedbreaks.pushBack(_fs->getCurrentPos());
			Lex();
			break;
		case TK_CONTINUE:
			if(_fs->_continuetargets.size() <= 0)error(_SC("'continue' has to be in a loop block"));
			if(_fs->_continuetargets.back() > 0) {
				_fs->addInstruction(_OP_POPTRAP, _fs->_continuetargets.back(), 0);
			}
			RESOLVE_OUTERS();
			_fs->addInstruction(_OP_JMP, 0, -1234);
			_fs->_unresolvedcontinues.pushBack(_fs->getCurrentPos());
			Lex();
			break;
		case TK_FUNCTION:
			FunctionStatement();
			break;
		case TK_CLASS:
			ClassStatement();
			break;
		case TK_ENUM:
			EnumStatement();
			break;
		case _SC('{'):{
				BEGIN_SCOPE();
				Lex();
				Statements();
				Expect(_SC('}'));
				if(closeframe) {
					END_SCOPE();
				}
				else {
					END_SCOPE_NO_CLOSE();
				}
			}
			break;
		case TK_TRY:
			TryCatchStatement();
			break;
		case TK_THROW:
			Lex();
			CommaExpr();
			_fs->addInstruction(_OP_THROW, _fs->popTarget());
			break;
		case TK_CONST:
			{
			Lex();
			rabbit::Object id = Expect(TK_IDENTIFIER);
			Expect('=');
			rabbit::Object val = ExpectScalar();
			OptionalSemicolon();
			SQTable *enums = _table(_get_shared_state(_vm)->_consts);
			rabbit::ObjectPtr strongid = id;
			enums->newSlot(strongid,rabbit::ObjectPtr(val));
			strongid.Null();
			}
			break;
		default:
			CommaExpr();
			_fs->discardTarget();
			//_fs->popTarget();
			break;
		}
		_fs->snoozeOpt();
	}
	void EmitDerefOp(SQOpcode op)
	{
		int64_t val = _fs->popTarget();
		int64_t key = _fs->popTarget();
		int64_t src = _fs->popTarget();
		_fs->addInstruction(op,_fs->pushTarget(),src,key,val);
	}
	void Emit2ArgsOP(SQOpcode op, int64_t p3 = 0)
	{
		int64_t p2 = _fs->popTarget(); //src in OP_GET
		int64_t p1 = _fs->popTarget(); //key in OP_GET
		_fs->addInstruction(op,_fs->pushTarget(), p1, p2, p3);
	}
	void EmitCompoundArith(int64_t tok, int64_t etype, int64_t pos)
	{
		/* Generate code depending on the expression type */
		switch(etype) {
		case LOCAL:{
			int64_t p2 = _fs->popTarget(); //src in OP_GET
			int64_t p1 = _fs->popTarget(); //key in OP_GET
			_fs->pushTarget(p1);
			//EmitCompArithLocal(tok, p1, p1, p2);
			_fs->addInstruction(ChooseArithOpByToken(tok),p1, p2, p1, 0);
			_fs->snoozeOpt();
				   }
			break;
		case OBJECT:
		case BASE:
			{
				int64_t val = _fs->popTarget();
				int64_t key = _fs->popTarget();
				int64_t src = _fs->popTarget();
				/* _OP_COMPARITH mixes dest obj and source val in the arg1 */
				_fs->addInstruction(_OP_COMPARITH, _fs->pushTarget(), (src<<16)|val, key, ChooseCompArithCharByToken(tok));
			}
			break;
		case OUTER:
			{
				int64_t val = _fs->topTarget();
				int64_t tmp = _fs->pushTarget();
				_fs->addInstruction(_OP_GETOUTER,   tmp, pos);
				_fs->addInstruction(ChooseArithOpByToken(tok), tmp, val, tmp, 0);
				_fs->popTarget();
				_fs->popTarget();
				_fs->addInstruction(_OP_SETOUTER, _fs->pushTarget(), pos, tmp);
			}
			break;
		}
	}
	void CommaExpr()
	{
		for(Expression();_token == ',';_fs->popTarget(), Lex(), CommaExpr());
	}
	void Expression()
	{
		 SQExpState es = _es;
		_es.etype	 = EXPR;
		_es.epos	  = -1;
		_es.donot_get = false;
		LogicalOrExp();
		switch(_token)  {
		case _SC('='):
		case TK_NEWSLOT:
		case TK_MINUSEQ:
		case TK_PLUSEQ:
		case TK_MULEQ:
		case TK_DIVEQ:
		case TK_MODEQ:{
			int64_t op = _token;
			int64_t ds = _es.etype;
			int64_t pos = _es.epos;
			if(ds == EXPR) error(_SC("can't assign expression"));
			else if(ds == BASE) error(_SC("'base' cannot be modified"));
			Lex(); Expression();

			switch(op){
			case TK_NEWSLOT:
				if(ds == OBJECT || ds == BASE)
					EmitDerefOp(_OP_NEWSLOT);
				else //if _derefstate != DEREF_NO_DEREF && DEREF_FIELD so is the index of a local
					error(_SC("can't 'create' a local slot"));
				break;
			case _SC('='): //ASSIGN
				switch(ds) {
				case LOCAL:
					{
						int64_t src = _fs->popTarget();
						int64_t dst = _fs->topTarget();
						_fs->addInstruction(_OP_MOVE, dst, src);
					}
					break;
				case OBJECT:
				case BASE:
					EmitDerefOp(_OP_SET);
					break;
				case OUTER:
					{
						int64_t src = _fs->popTarget();
						int64_t dst = _fs->pushTarget();
						_fs->addInstruction(_OP_SETOUTER, dst, pos, src);
					}
				}
				break;
			case TK_MINUSEQ:
			case TK_PLUSEQ:
			case TK_MULEQ:
			case TK_DIVEQ:
			case TK_MODEQ:
				EmitCompoundArith(op, ds, pos);
				break;
			}
			}
			break;
		case _SC('?'): {
			Lex();
			_fs->addInstruction(_OP_JZ, _fs->popTarget());
			int64_t jzpos = _fs->getCurrentPos();
			int64_t trg = _fs->pushTarget();
			Expression();
			int64_t first_exp = _fs->popTarget();
			if(trg != first_exp) _fs->addInstruction(_OP_MOVE, trg, first_exp);
			int64_t endfirstexp = _fs->getCurrentPos();
			_fs->addInstruction(_OP_JMP, 0, 0);
			Expect(_SC(':'));
			int64_t jmppos = _fs->getCurrentPos();
			Expression();
			int64_t second_exp = _fs->popTarget();
			if(trg != second_exp) _fs->addInstruction(_OP_MOVE, trg, second_exp);
			_fs->setIntructionParam(jmppos, 1, _fs->getCurrentPos() - jmppos);
			_fs->setIntructionParam(jzpos, 1, endfirstexp - jzpos + 1);
			_fs->snoozeOpt();
			}
			break;
		}
		_es = es;
	}
	template<typename T> void INVOKE_EXP(T f)
	{
		SQExpState es = _es;
		_es.etype	 = EXPR;
		_es.epos	  = -1;
		_es.donot_get = false;
		(this->*f)();
		_es = es;
	}
	template<typename T> void BIN_EXP(SQOpcode op, T f,int64_t op3 = 0)
	{
		Lex();
		INVOKE_EXP(f);
		int64_t op1 = _fs->popTarget();int64_t op2 = _fs->popTarget();
		_fs->addInstruction(op, _fs->pushTarget(), op1, op2, op3);
		_es.etype = EXPR;
	}
	void LogicalOrExp()
	{
		LogicalAndExp();
		for(;;) if(_token == TK_OR) {
			int64_t first_exp = _fs->popTarget();
			int64_t trg = _fs->pushTarget();
			_fs->addInstruction(_OP_OR, trg, 0, first_exp, 0);
			int64_t jpos = _fs->getCurrentPos();
			if(trg != first_exp) _fs->addInstruction(_OP_MOVE, trg, first_exp);
			Lex(); INVOKE_EXP(&SQcompiler::LogicalOrExp);
			_fs->snoozeOpt();
			int64_t second_exp = _fs->popTarget();
			if(trg != second_exp) _fs->addInstruction(_OP_MOVE, trg, second_exp);
			_fs->snoozeOpt();
			_fs->setIntructionParam(jpos, 1, (_fs->getCurrentPos() - jpos));
			_es.etype = EXPR;
			break;
		}else return;
	}
	void LogicalAndExp()
	{
		BitwiseOrExp();
		for(;;) switch(_token) {
		case TK_AND: {
			int64_t first_exp = _fs->popTarget();
			int64_t trg = _fs->pushTarget();
			_fs->addInstruction(_OP_AND, trg, 0, first_exp, 0);
			int64_t jpos = _fs->getCurrentPos();
			if(trg != first_exp) _fs->addInstruction(_OP_MOVE, trg, first_exp);
			Lex(); INVOKE_EXP(&SQcompiler::LogicalAndExp);
			_fs->snoozeOpt();
			int64_t second_exp = _fs->popTarget();
			if(trg != second_exp) _fs->addInstruction(_OP_MOVE, trg, second_exp);
			_fs->snoozeOpt();
			_fs->setIntructionParam(jpos, 1, (_fs->getCurrentPos() - jpos));
			_es.etype = EXPR;
			break;
			}

		default:
			return;
		}
	}
	void BitwiseOrExp()
	{
		BitwiseXorExp();
		for(;;) if(_token == _SC('|'))
		{BIN_EXP(_OP_BITW, &SQcompiler::BitwiseXorExp,BW_OR);
		}else return;
	}
	void BitwiseXorExp()
	{
		BitwiseAndExp();
		for(;;) if(_token == _SC('^'))
		{BIN_EXP(_OP_BITW, &SQcompiler::BitwiseAndExp,BW_XOR);
		}else return;
	}
	void BitwiseAndExp()
	{
		EqExp();
		for(;;) if(_token == _SC('&'))
		{BIN_EXP(_OP_BITW, &SQcompiler::EqExp,BW_AND);
		}else return;
	}
	void EqExp()
	{
		CompExp();
		for(;;) switch(_token) {
		case TK_EQ: BIN_EXP(_OP_EQ, &SQcompiler::CompExp); break;
		case TK_NE: BIN_EXP(_OP_NE, &SQcompiler::CompExp); break;
		case TK_3WAYSCMP: BIN_EXP(_OP_CMP, &SQcompiler::CompExp,CMP_3W); break;
		default: return;
		}
	}
	void CompExp()
	{
		ShiftExp();
		for(;;) switch(_token) {
		case _SC('>'): BIN_EXP(_OP_CMP, &SQcompiler::ShiftExp,CMP_G); break;
		case _SC('<'): BIN_EXP(_OP_CMP, &SQcompiler::ShiftExp,CMP_L); break;
		case TK_GE: BIN_EXP(_OP_CMP, &SQcompiler::ShiftExp,CMP_GE); break;
		case TK_LE: BIN_EXP(_OP_CMP, &SQcompiler::ShiftExp,CMP_LE); break;
		case TK_IN: BIN_EXP(_OP_EXISTS, &SQcompiler::ShiftExp); break;
		case TK_INSTANCEOF: BIN_EXP(_OP_INSTANCEOF, &SQcompiler::ShiftExp); break;
		default: return;
		}
	}
	void ShiftExp()
	{
		PlusExp();
		for(;;) switch(_token) {
		case TK_USHIFTR: BIN_EXP(_OP_BITW, &SQcompiler::PlusExp,BW_USHIFTR); break;
		case TK_SHIFTL: BIN_EXP(_OP_BITW, &SQcompiler::PlusExp,BW_SHIFTL); break;
		case TK_SHIFTR: BIN_EXP(_OP_BITW, &SQcompiler::PlusExp,BW_SHIFTR); break;
		default: return;
		}
	}
	SQOpcode ChooseArithOpByToken(int64_t tok)
	{
		switch(tok) {
			case TK_PLUSEQ: case '+': return _OP_ADD;
			case TK_MINUSEQ: case '-': return _OP_SUB;
			case TK_MULEQ: case '*': return _OP_MUL;
			case TK_DIVEQ: case '/': return _OP_DIV;
			case TK_MODEQ: case '%': return _OP_MOD;
			default: assert(0);
		}
		return _OP_ADD;
	}
	int64_t ChooseCompArithCharByToken(int64_t tok)
	{
		int64_t oper;
		switch(tok){
		case TK_MINUSEQ: oper = '-'; break;
		case TK_PLUSEQ: oper = '+'; break;
		case TK_MULEQ: oper = '*'; break;
		case TK_DIVEQ: oper = '/'; break;
		case TK_MODEQ: oper = '%'; break;
		default: oper = 0; //shut up compiler
			assert(0); break;
		};
		return oper;
	}
	void PlusExp()
	{
		MultExp();
		for(;;) switch(_token) {
		case _SC('+'): case _SC('-'):
			BIN_EXP(ChooseArithOpByToken(_token), &SQcompiler::MultExp); break;
		default: return;
		}
	}

	void MultExp()
	{
		PrefixedExpr();
		for(;;) switch(_token) {
		case _SC('*'): case _SC('/'): case _SC('%'):
			BIN_EXP(ChooseArithOpByToken(_token), &SQcompiler::PrefixedExpr); break;
		default: return;
		}
	}
	//if 'pos' != -1 the previous variable is a local variable
	void PrefixedExpr()
	{
		int64_t pos = Factor();
		for(;;) {
			switch(_token) {
			case _SC('.'):
				pos = -1;
				Lex();

				_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(Expect(TK_IDENTIFIER)));
				if(_es.etype==BASE) {
					Emit2ArgsOP(_OP_GET);
					pos = _fs->topTarget();
					_es.etype = EXPR;
					_es.epos   = pos;
				}
				else {
					if(Needget()) {
						Emit2ArgsOP(_OP_GET);
					}
					_es.etype = OBJECT;
				}
				break;
			case _SC('['):
				if(_lex._prevtoken == _SC('\n')) error(_SC("cannot brake deref/or comma needed after [exp]=exp slot declaration"));
				Lex(); Expression(); Expect(_SC(']'));
				pos = -1;
				if(_es.etype==BASE) {
					Emit2ArgsOP(_OP_GET);
					pos = _fs->topTarget();
					_es.etype = EXPR;
					_es.epos   = pos;
				}
				else {
					if(Needget()) {
						Emit2ArgsOP(_OP_GET);
					}
					_es.etype = OBJECT;
				}
				break;
			case TK_MINUSMINUS:
			case TK_PLUSPLUS:
				{
					if(IsEndOfStatement()) return;
					int64_t diff = (_token==TK_MINUSMINUS) ? -1 : 1;
					Lex();
					switch(_es.etype)
					{
						case EXPR: error(_SC("can't '++' or '--' an expression")); break;
						case OBJECT:
						case BASE:
							if(_es.donot_get == true)  { error(_SC("can't '++' or '--' an expression")); break; } //mmh dor this make sense?
							Emit2ArgsOP(_OP_PINC, diff);
							break;
						case LOCAL: {
							int64_t src = _fs->popTarget();
							_fs->addInstruction(_OP_PINCL, _fs->pushTarget(), src, 0, diff);
									}
							break;
						case OUTER: {
							int64_t tmp1 = _fs->pushTarget();
							int64_t tmp2 = _fs->pushTarget();
							_fs->addInstruction(_OP_GETOUTER, tmp2, _es.epos);
							_fs->addInstruction(_OP_PINCL,	tmp1, tmp2, 0, diff);
							_fs->addInstruction(_OP_SETOUTER, tmp2, _es.epos, tmp2);
							_fs->popTarget();
						}
					}
				}
				return;
				break;
			case _SC('('):
				switch(_es.etype) {
					case OBJECT: {
						int64_t key	 = _fs->popTarget();  /* location of the key */
						int64_t table   = _fs->popTarget();  /* location of the object */
						int64_t closure = _fs->pushTarget(); /* location for the closure */
						int64_t ttarget = _fs->pushTarget(); /* location for 'this' pointer */
						_fs->addInstruction(_OP_PREPCALL, closure, key, table, ttarget);
						}
						break;
					case BASE:
						//Emit2ArgsOP(_OP_GET);
						_fs->addInstruction(_OP_MOVE, _fs->pushTarget(), 0);
						break;
					case OUTER:
						_fs->addInstruction(_OP_GETOUTER, _fs->pushTarget(), _es.epos);
						_fs->addInstruction(_OP_MOVE,	 _fs->pushTarget(), 0);
						break;
					default:
						_fs->addInstruction(_OP_MOVE, _fs->pushTarget(), 0);
				}
				_es.etype = EXPR;
				Lex();
				FunctioncallArgs();
				break;
			default: return;
			}
		}
	}
	int64_t Factor()
	{
		//_es.etype = EXPR;
		switch(_token)
		{
		case TK_STRING_LITERAL:
			_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(_fs->createString(_lex._svalue,_lex._longstr.size()-1)));
			Lex();
			break;
		case TK_BASE:
			Lex();
			_fs->addInstruction(_OP_GETBASE, _fs->pushTarget());
			_es.etype  = BASE;
			_es.epos   = _fs->topTarget();
			return (_es.epos);
			break;
		case TK_IDENTIFIER:
		case TK_CONSTRUCTOR:
		case TK_THIS:{
				rabbit::Object id;
				rabbit::Object constant;

				switch(_token) {
					case TK_IDENTIFIER:  id = _fs->createString(_lex._svalue);	   break;
					case TK_THIS:		id = _fs->createString(_SC("this"),4);		break;
					case TK_CONSTRUCTOR: id = _fs->createString(_SC("constructor"),11); break;
				}

				int64_t pos = -1;
				Lex();
				if((pos = _fs->getLocalVariable(id)) != -1) {
					/* Handle a local variable (includes 'this') */
					_fs->pushTarget(pos);
					_es.etype  = LOCAL;
					_es.epos   = pos;
				}

				else if((pos = _fs->getOuterVariable(id)) != -1) {
					/* Handle a free var */
					if(Needget()) {
						_es.epos  = _fs->pushTarget();
						_fs->addInstruction(_OP_GETOUTER, _es.epos, pos);
						/* _es.etype = EXPR; already default value */
					}
					else {
						_es.etype = OUTER;
						_es.epos  = pos;
					}
				}

				else if(_fs->isConstant(id, constant)) {
					/* Handle named constant */
					rabbit::ObjectPtr constval;
					rabbit::Object	constid;
					if(sq_type(constant) == rabbit::OT_TABLE) {
						Expect('.');
						constid = Expect(TK_IDENTIFIER);
						if(!_table(constant)->get(constid, constval)) {
							constval.Null();
							error(_SC("invalid constant [%s.%s]"), _stringval(id), _stringval(constid));
						}
					}
					else {
						constval = constant;
					}
					_es.epos = _fs->pushTarget();

					/* generate direct or literal function depending on size */
					rabbit::ObjectType ctype = sq_type(constval);
					switch(ctype) {
						case rabbit::OT_INTEGER: EmitloadConstInt(_integer(constval),_es.epos); break;
						case rabbit::OT_FLOAT: EmitloadConstFloat(_float(constval),_es.epos); break;
						case rabbit::OT_BOOL: _fs->addInstruction(_OP_LOADBOOL, _es.epos, _integer(constval)); break;
						default: _fs->addInstruction(_OP_LOAD,_es.epos,_fs->getConstant(constval)); break;
					}
					_es.etype = EXPR;
				}
				else {
					/* Handle a non-local variable, aka a field. push the 'this' pointer on
					* the virtual stack (always found in offset 0, so no instruction needs to
					* be generated), and push the key next. Generate an _OP_LOAD instruction
					* for the latter. If we are not using the variable as a dref expr, generate
					* the _OP_GET instruction.
					*/
					_fs->pushTarget(0);
					_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(id));
					if(Needget()) {
						Emit2ArgsOP(_OP_GET);
					}
					_es.etype = OBJECT;
				}
				return _es.epos;
			}
			break;
		case TK_DOUBLE_COLON:  // "::"
			_fs->addInstruction(_OP_LOADROOT, _fs->pushTarget());
			_es.etype = OBJECT;
			_token = _SC('.'); /* hack: drop into PrefixExpr, case '.'*/
			_es.epos = -1;
			return _es.epos;
			break;
		case TK_NULL:
			_fs->addInstruction(_OP_LOADNULLS, _fs->pushTarget(),1);
			Lex();
			break;
		case TK_INTEGER: EmitloadConstInt(_lex._nvalue,-1); Lex();  break;
		case TK_FLOAT: EmitloadConstFloat(_lex._fvalue,-1); Lex(); break;
		case TK_TRUE: case TK_FALSE:
			_fs->addInstruction(_OP_LOADBOOL, _fs->pushTarget(),_token == TK_TRUE?1:0);
			Lex();
			break;
		case _SC('['): {
				_fs->addInstruction(_OP_NEWOBJ, _fs->pushTarget(),0,0,NOT_ARRAY);
				int64_t apos = _fs->getCurrentPos(),key = 0;
				Lex();
				while(_token != _SC(']')) {
					Expression();
					if(_token == _SC(',')) Lex();
					int64_t val = _fs->popTarget();
					int64_t array = _fs->topTarget();
					_fs->addInstruction(_OP_APPENDARRAY, array, val, AAT_STACK);
					key++;
				}
				_fs->setIntructionParam(apos, 1, key);
				Lex();
			}
			break;
		case _SC('{'):
			_fs->addInstruction(_OP_NEWOBJ, _fs->pushTarget(),0,NOT_TABLE);
			Lex();ParseTableOrClass(_SC(','),_SC('}'));
			break;
		case TK_FUNCTION: FunctionExp(_token);break;
		case _SC('@'): FunctionExp(_token,true);break;
		case TK_CLASS: Lex(); ClassExp();break;
		case _SC('-'):
			Lex();
			switch(_token) {
			case TK_INTEGER: EmitloadConstInt(-_lex._nvalue,-1); Lex(); break;
			case TK_FLOAT: EmitloadConstFloat(-_lex._fvalue,-1); Lex(); break;
			default: UnaryOP(_OP_NEG);
			}
			break;
		case _SC('!'): Lex(); UnaryOP(_OP_NOT); break;
		case _SC('~'):
			Lex();
			if(_token == TK_INTEGER)  { EmitloadConstInt(~_lex._nvalue,-1); Lex(); break; }
			UnaryOP(_OP_BWNOT);
			break;
		case TK_TYPEOF : Lex() ;UnaryOP(_OP_TYPEOF); break;
		case TK_RESUME : Lex(); UnaryOP(_OP_RESUME); break;
		case TK_CLONE : Lex(); UnaryOP(_OP_CLONE); break;
		case TK_RAWCALL: Lex(); Expect('('); FunctioncallArgs(true); break;
		case TK_MINUSMINUS :
		case TK_PLUSPLUS :PrefixIncDec(_token); break;
		case TK_DELETE : DeleteExpr(); break;
		case _SC('('): Lex(); CommaExpr(); Expect(_SC(')'));
			break;
		case TK___LINE__: EmitloadConstInt(_lex._currentline,-1); Lex(); break;
		case TK___FILE__: _fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(_sourcename)); Lex(); break;
		default: error(_SC("expression expected"));
		}
		_es.etype = EXPR;
		return -1;
	}
	void EmitloadConstInt(int64_t value,int64_t target)
	{
		if(target < 0) {
			target = _fs->pushTarget();
		}
		if(value <= INT_MAX && value > INT_MIN) { //does it fit in 32 bits?
			_fs->addInstruction(_OP_LOADINT, target,value);
		}
		else {
			_fs->addInstruction(_OP_LOAD, target, _fs->getNumericConstant(value));
		}
	}
	void EmitloadConstFloat(float_t value,int64_t target)
	{
		if(target < 0) {
			target = _fs->pushTarget();
		}
		if(sizeof(float_t) == sizeof(int32_t)) {
			_fs->addInstruction(_OP_LOADFLOAT, target,*((int32_t *)&value));
		}
		else {
			_fs->addInstruction(_OP_LOAD, target, _fs->getNumericConstant(value));
		}
	}
	void UnaryOP(SQOpcode op)
	{
		PrefixedExpr();
		int64_t src = _fs->popTarget();
		_fs->addInstruction(op, _fs->pushTarget(), src);
	}
	bool Needget()
	{
		switch(_token) {
		case _SC('='): case _SC('('): case TK_NEWSLOT: case TK_MODEQ: case TK_MULEQ:
		case TK_DIVEQ: case TK_MINUSEQ: case TK_PLUSEQ:
			return false;
		case TK_PLUSPLUS: case TK_MINUSMINUS:
			if (!IsEndOfStatement()) {
				return false;
			}
		break;
		}
		return (!_es.donot_get || ( _es.donot_get && (_token == _SC('.') || _token == _SC('['))));
	}
	void FunctioncallArgs(bool rawcall = false)
	{
		int64_t nargs = 1;//this
		 while(_token != _SC(')')) {
			 Expression();
			 MoveIfCurrentTargetisLocal();
			 nargs++;
			 if(_token == _SC(',')){
				 Lex();
				 if(_token == ')') error(_SC("expression expected, found ')'"));
			 }
		 }
		 Lex();
		 if (rawcall) {
			 if (nargs < 3) error(_SC("rawcall requires at least 2 parameters (callee and this)"));
			 nargs -= 2; //removes callee and this from count
		 }
		 for(int64_t i = 0; i < (nargs - 1); i++) _fs->popTarget();
		 int64_t stackbase = _fs->popTarget();
		 int64_t closure = _fs->popTarget();
		 _fs->addInstruction(_OP_CALL, _fs->pushTarget(), closure, stackbase, nargs);
	}
	void ParseTableOrClass(int64_t separator,int64_t terminator)
	{
		int64_t tpos = _fs->getCurrentPos(),nkeys = 0;
		while(_token != terminator) {
			bool hasattrs = false;
			bool isstatic = false;
			//check if is an attribute
			if(separator == ';') {
				if(_token == TK_ATTR_OPEN) {
					_fs->addInstruction(_OP_NEWOBJ, _fs->pushTarget(),0,NOT_TABLE); Lex();
					ParseTableOrClass(',',TK_ATTR_CLOSE);
					hasattrs = true;
				}
				if(_token == TK_STATIC) {
					isstatic = true;
					Lex();
				}
			}
			switch(_token) {
			case TK_FUNCTION:
			case TK_CONSTRUCTOR:{
				int64_t tk = _token;
				Lex();
				rabbit::Object id = tk == TK_FUNCTION ? Expect(TK_IDENTIFIER) : _fs->createString(_SC("constructor"));
				Expect(_SC('('));
				_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(id));
				createFunction(id);
				_fs->addInstruction(_OP_CLOSURE, _fs->pushTarget(), _fs->_functions.size() - 1, 0);
								}
								break;
			case _SC('['):
				Lex(); CommaExpr(); Expect(_SC(']'));
				Expect(_SC('=')); Expression();
				break;
			case TK_STRING_LITERAL: //JSON
				if(separator == ',') { //only works for tables
					_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(Expect(TK_STRING_LITERAL)));
					Expect(_SC(':')); Expression();
					break;
				}
			default :
				_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(Expect(TK_IDENTIFIER)));
				Expect(_SC('=')); Expression();
			}
			if(_token == separator) Lex();//optional comma/semicolon
			nkeys++;
			int64_t val = _fs->popTarget();
			int64_t key = _fs->popTarget();
			int64_t attrs = hasattrs ? _fs->popTarget():-1;
			((void)attrs);
			assert((hasattrs && (attrs == key-1)) || !hasattrs);
			unsigned char flags = (hasattrs?NEW_SLOT_ATTRIBUTES_FLAG:0)|(isstatic?NEW_SLOT_STATIC_FLAG:0);
			int64_t table = _fs->topTarget(); //<<BECAUSE OF THIS NO COMMON EMIT FUNC IS POSSIBLE
			if(separator == _SC(',')) { //hack recognizes a table from the separator
				_fs->addInstruction(_OP_NEWSLOT, 0xFF, table, key, val);
			}
			else {
				_fs->addInstruction(_OP_NEWSLOTA, flags, table, key, val); //this for classes only as it invokes _newmember
			}
		}
		if(separator == _SC(',')) //hack recognizes a table from the separator
			_fs->setIntructionParam(tpos, 1, nkeys);
		Lex();
	}
	void LocalDeclStatement()
	{
		rabbit::Object varname;
		Lex();
		if( _token == TK_FUNCTION) {
			Lex();
			varname = Expect(TK_IDENTIFIER);
			Expect(_SC('('));
			createFunction(varname,false);
			_fs->addInstruction(_OP_CLOSURE, _fs->pushTarget(), _fs->_functions.size() - 1, 0);
			_fs->popTarget();
			_fs->pushLocalVariable(varname);
			return;
		}

		do {
			varname = Expect(TK_IDENTIFIER);
			if(_token == _SC('=')) {
				Lex(); Expression();
				int64_t src = _fs->popTarget();
				int64_t dest = _fs->pushTarget();
				if(dest != src) _fs->addInstruction(_OP_MOVE, dest, src);
			}
			else{
				_fs->addInstruction(_OP_LOADNULLS, _fs->pushTarget(),1);
			}
			_fs->popTarget();
			_fs->pushLocalVariable(varname);
			if(_token == _SC(',')) Lex(); else break;
		} while(1);
	}
	void IfBlock()
	{
		if (_token == _SC('{'))
		{
			BEGIN_SCOPE();
			Lex();
			Statements();
			Expect(_SC('}'));
			if (true) {
				END_SCOPE();
			}
			else {
				END_SCOPE_NO_CLOSE();
			}
		}
		else {
			//BEGIN_SCOPE();
			Statement();
			if (_lex._prevtoken != _SC('}') && _lex._prevtoken != _SC(';')) OptionalSemicolon();
			//END_SCOPE();
		}
	}
	void IfStatement()
	{
		int64_t jmppos;
		bool haselse = false;
		Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		_fs->addInstruction(_OP_JZ, _fs->popTarget());
		int64_t jnepos = _fs->getCurrentPos();



		IfBlock();
		//
		/*static int n = 0;
		if (_token != _SC('}') && _token != TK_ELSE) {
			printf("IF %d-----------------------!!!!!!!!!\n", n);
			if (n == 5)
			{
				printf("asd");
			}
			n++;
			//OptionalSemicolon();
		}*/


		int64_t endifblock = _fs->getCurrentPos();
		if(_token == TK_ELSE){
			haselse = true;
			//BEGIN_SCOPE();
			_fs->addInstruction(_OP_JMP);
			jmppos = _fs->getCurrentPos();
			Lex();
			//Statement(); if(_lex._prevtoken != _SC('}')) OptionalSemicolon();
			IfBlock();
			//END_SCOPE();
			_fs->setIntructionParam(jmppos, 1, _fs->getCurrentPos() - jmppos);
		}
		_fs->setIntructionParam(jnepos, 1, endifblock - jnepos + (haselse?1:0));
	}
	void WhileStatement()
	{
		int64_t jzpos, jmppos;
		jmppos = _fs->getCurrentPos();
		Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));

		BEGIN_BREAKBLE_BLOCK();
		_fs->addInstruction(_OP_JZ, _fs->popTarget());
		jzpos = _fs->getCurrentPos();
		BEGIN_SCOPE();

		Statement();

		END_SCOPE();
		_fs->addInstruction(_OP_JMP, 0, jmppos - _fs->getCurrentPos() - 1);
		_fs->setIntructionParam(jzpos, 1, _fs->getCurrentPos() - jzpos);

		END_BREAKBLE_BLOCK(jmppos);
	}
	void DoWhileStatement()
	{
		Lex();
		int64_t jmptrg = _fs->getCurrentPos();
		BEGIN_BREAKBLE_BLOCK()
		BEGIN_SCOPE();
		Statement();
		END_SCOPE();
		Expect(TK_WHILE);
		int64_t continuetrg = _fs->getCurrentPos();
		Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		_fs->addInstruction(_OP_JZ, _fs->popTarget(), 1);
		_fs->addInstruction(_OP_JMP, 0, jmptrg - _fs->getCurrentPos() - 1);
		END_BREAKBLE_BLOCK(continuetrg);
	}
	void ForStatement()
	{
		Lex();
		BEGIN_SCOPE();
		Expect(_SC('('));
		if(_token == TK_LOCAL) LocalDeclStatement();
		else if(_token != _SC(';')){
			CommaExpr();
			_fs->popTarget();
		}
		Expect(_SC(';'));
		_fs->snoozeOpt();
		int64_t jmppos = _fs->getCurrentPos();
		int64_t jzpos = -1;
		if(_token != _SC(';')) { CommaExpr(); _fs->addInstruction(_OP_JZ, _fs->popTarget()); jzpos = _fs->getCurrentPos(); }
		Expect(_SC(';'));
		_fs->snoozeOpt();
		int64_t expstart = _fs->getCurrentPos() + 1;
		if(_token != _SC(')')) {
			CommaExpr();
			_fs->popTarget();
		}
		Expect(_SC(')'));
		_fs->snoozeOpt();
		int64_t expend = _fs->getCurrentPos();
		int64_t expsize = (expend - expstart) + 1;
		SQInstructionVec exp;
		if(expsize > 0) {
			for(int64_t i = 0; i < expsize; i++)
				exp.pushBack(_fs->getInstruction(expstart + i));
			_fs->popInstructions(expsize);
		}
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		int64_t continuetrg = _fs->getCurrentPos();
		if(expsize > 0) {
			for(int64_t i = 0; i < expsize; i++)
				_fs->addInstruction(exp[i]);
		}
		_fs->addInstruction(_OP_JMP, 0, jmppos - _fs->getCurrentPos() - 1, 0);
		if(jzpos>  0) _fs->setIntructionParam(jzpos, 1, _fs->getCurrentPos() - jzpos);
		
		END_BREAKBLE_BLOCK(continuetrg);

		END_SCOPE();
	}
	void ForEachStatement()
	{
		rabbit::Object idxname, valname;
		Lex(); Expect(_SC('(')); valname = Expect(TK_IDENTIFIER);
		if(_token == _SC(',')) {
			idxname = valname;
			Lex(); valname = Expect(TK_IDENTIFIER);
		}
		else{
			idxname = _fs->createString(_SC("@INDEX@"));
		}
		Expect(TK_IN);

		//save the stack size
		BEGIN_SCOPE();
		//put the table in the stack(evaluate the table expression)
		Expression(); Expect(_SC(')'));
		int64_t container = _fs->topTarget();
		//push the index local var
		int64_t indexpos = _fs->pushLocalVariable(idxname);
		_fs->addInstruction(_OP_LOADNULLS, indexpos,1);
		//push the value local var
		int64_t valuepos = _fs->pushLocalVariable(valname);
		_fs->addInstruction(_OP_LOADNULLS, valuepos,1);
		//push reference index
		int64_t itrpos = _fs->pushLocalVariable(_fs->createString(_SC("@ITERATOR@"))); //use invalid id to make it inaccessible
		_fs->addInstruction(_OP_LOADNULLS, itrpos,1);
		int64_t jmppos = _fs->getCurrentPos();
		_fs->addInstruction(_OP_FOREACH, container, 0, indexpos);
		int64_t foreachpos = _fs->getCurrentPos();
		_fs->addInstruction(_OP_POSTFOREACH, container, 0, indexpos);
		//generate the statement code
		BEGIN_BREAKBLE_BLOCK()
		Statement();
		_fs->addInstruction(_OP_JMP, 0, jmppos - _fs->getCurrentPos() - 1);
		_fs->setIntructionParam(foreachpos, 1, _fs->getCurrentPos() - foreachpos);
		_fs->setIntructionParam(foreachpos + 1, 1, _fs->getCurrentPos() - foreachpos);
		END_BREAKBLE_BLOCK(foreachpos - 1);
		//restore the local variable stack(remove index,val and ref idx)
		_fs->popTarget();
		END_SCOPE();
	}
	void SwitchStatement()
	{
		Lex(); Expect(_SC('(')); CommaExpr(); Expect(_SC(')'));
		Expect(_SC('{'));
		int64_t expr = _fs->topTarget();
		bool bfirst = true;
		int64_t tonextcondjmp = -1;
		int64_t skipcondjmp = -1;
		int64_t __nbreaks__ = _fs->_unresolvedbreaks.size();
		_fs->_breaktargets.pushBack(0);
		while(_token == TK_CASE) {
			if(!bfirst) {
				_fs->addInstruction(_OP_JMP, 0, 0);
				skipcondjmp = _fs->getCurrentPos();
				_fs->setIntructionParam(tonextcondjmp, 1, _fs->getCurrentPos() - tonextcondjmp);
			}
			//condition
			Lex(); Expression(); Expect(_SC(':'));
			int64_t trg = _fs->popTarget();
			int64_t eqtarget = trg;
			bool local = _fs->isLocal(trg);
			if(local) {
				eqtarget = _fs->pushTarget(); //we need to allocate a extra reg
			}
			_fs->addInstruction(_OP_EQ, eqtarget, trg, expr);
			_fs->addInstruction(_OP_JZ, eqtarget, 0);
			if(local) {
				_fs->popTarget();
			}

			//end condition
			if(skipcondjmp != -1) {
				_fs->setIntructionParam(skipcondjmp, 1, (_fs->getCurrentPos() - skipcondjmp));
			}
			tonextcondjmp = _fs->getCurrentPos();
			BEGIN_SCOPE();
			Statements();
			END_SCOPE();
			bfirst = false;
		}
		if(tonextcondjmp != -1)
			_fs->setIntructionParam(tonextcondjmp, 1, _fs->getCurrentPos() - tonextcondjmp);
		if(_token == TK_DEFAULT) {
			Lex(); Expect(_SC(':'));
			BEGIN_SCOPE();
			Statements();
			END_SCOPE();
		}
		Expect(_SC('}'));
		_fs->popTarget();
		__nbreaks__ = _fs->_unresolvedbreaks.size() - __nbreaks__;
		if(__nbreaks__ > 0)ResolveBreaks(_fs, __nbreaks__);
		_fs->_breaktargets.popBack();
	}
	void FunctionStatement()
	{
		rabbit::Object id;
		Lex(); id = Expect(TK_IDENTIFIER);
		_fs->pushTarget(0);
		_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(id));
		if(_token == TK_DOUBLE_COLON) Emit2ArgsOP(_OP_GET);

		while(_token == TK_DOUBLE_COLON) {
			Lex();
			id = Expect(TK_IDENTIFIER);
			_fs->addInstruction(_OP_LOAD, _fs->pushTarget(), _fs->getConstant(id));
			if(_token == TK_DOUBLE_COLON) Emit2ArgsOP(_OP_GET);
		}
		Expect(_SC('('));
		createFunction(id);
		_fs->addInstruction(_OP_CLOSURE, _fs->pushTarget(), _fs->_functions.size() - 1, 0);
		EmitDerefOp(_OP_NEWSLOT);
		_fs->popTarget();
	}
	void ClassStatement()
	{
		SQExpState es;
		Lex();
		es = _es;
		_es.donot_get = true;
		PrefixedExpr();
		if(_es.etype == EXPR) {
			error(_SC("invalid class name"));
		}
		else if(_es.etype == OBJECT || _es.etype == BASE) {
			ClassExp();
			EmitDerefOp(_OP_NEWSLOT);
			_fs->popTarget();
		}
		else {
			error(_SC("cannot create a class in a local with the syntax(class <local>)"));
		}
		_es = es;
	}
	rabbit::Object ExpectScalar()
	{
		rabbit::Object val;
		val._type = rabbit::OT_NULL; val._unVal.nInteger = 0; //shut up GCC 4.x
		switch(_token) {
			case TK_INTEGER:
				val._type = rabbit::OT_INTEGER;
				val._unVal.nInteger = _lex._nvalue;
				break;
			case TK_FLOAT:
				val._type = rabbit::OT_FLOAT;
				val._unVal.fFloat = _lex._fvalue;
				break;
			case TK_STRING_LITERAL:
				val = _fs->createString(_lex._svalue,_lex._longstr.size()-1);
				break;
			case TK_TRUE:
			case TK_FALSE:
				val._type = rabbit::OT_BOOL;
				val._unVal.nInteger = _token == TK_TRUE ? 1 : 0;
				break;
			case '-':
				Lex();
				switch(_token)
				{
				case TK_INTEGER:
					val._type = rabbit::OT_INTEGER;
					val._unVal.nInteger = -_lex._nvalue;
				break;
				case TK_FLOAT:
					val._type = rabbit::OT_FLOAT;
					val._unVal.fFloat = -_lex._fvalue;
				break;
				default:
					error(_SC("scalar expected : integer, float"));
				}
				break;
			default:
				error(_SC("scalar expected : integer, float, or string"));
		}
		Lex();
		return val;
	}
	void EnumStatement()
	{
		Lex();
		rabbit::Object id = Expect(TK_IDENTIFIER);
		Expect(_SC('{'));

		rabbit::Object table = _fs->createTable();
		int64_t nval = 0;
		while(_token != _SC('}')) {
			rabbit::Object key = Expect(TK_IDENTIFIER);
			rabbit::Object val;
			if(_token == _SC('=')) {
				Lex();
				val = ExpectScalar();
			}
			else {
				val._type = rabbit::OT_INTEGER;
				val._unVal.nInteger = nval++;
			}
			_table(table)->newSlot(rabbit::ObjectPtr(key),rabbit::ObjectPtr(val));
			if(_token == ',') Lex();
		}
		SQTable *enums = _table(_get_shared_state(_vm)->_consts);
		rabbit::ObjectPtr strongid = id;
		enums->newSlot(rabbit::ObjectPtr(strongid),rabbit::ObjectPtr(table));
		strongid.Null();
		Lex();
	}
	void TryCatchStatement()
	{
		rabbit::Object exid;
		Lex();
		_fs->addInstruction(_OP_PUSHTRAP,0,0);
		_fs->_traps++;
		if(_fs->_breaktargets.size()) _fs->_breaktargets.back()++;
		if(_fs->_continuetargets.size()) _fs->_continuetargets.back()++;
		int64_t trappos = _fs->getCurrentPos();
		{
			BEGIN_SCOPE();
			Statement();
			END_SCOPE();
		}
		_fs->_traps--;
		_fs->addInstruction(_OP_POPTRAP, 1, 0);
		if(_fs->_breaktargets.size()) _fs->_breaktargets.back()--;
		if(_fs->_continuetargets.size()) _fs->_continuetargets.back()--;
		_fs->addInstruction(_OP_JMP, 0, 0);
		int64_t jmppos = _fs->getCurrentPos();
		_fs->setIntructionParam(trappos, 1, (_fs->getCurrentPos() - trappos));
		Expect(TK_CATCH); Expect(_SC('(')); exid = Expect(TK_IDENTIFIER); Expect(_SC(')'));
		{
			BEGIN_SCOPE();
			int64_t ex_target = _fs->pushLocalVariable(exid);
			_fs->setIntructionParam(trappos, 0, ex_target);
			Statement();
			_fs->setIntructionParams(jmppos, 0, (_fs->getCurrentPos() - jmppos), 0);
			END_SCOPE();
		}
	}
	void FunctionExp(int64_t ftype,bool lambda = false)
	{
		Lex(); Expect(_SC('('));
		rabbit::ObjectPtr dummy;
		createFunction(dummy,lambda);
		_fs->addInstruction(_OP_CLOSURE, _fs->pushTarget(), _fs->_functions.size() - 1, ftype == TK_FUNCTION?0:1);
	}
	void ClassExp()
	{
		int64_t base = -1;
		int64_t attrs = -1;
		if(_token == TK_EXTENDS) {
			Lex(); Expression();
			base = _fs->topTarget();
		}
		if(_token == TK_ATTR_OPEN) {
			Lex();
			_fs->addInstruction(_OP_NEWOBJ, _fs->pushTarget(),0,NOT_TABLE);
			ParseTableOrClass(_SC(','),TK_ATTR_CLOSE);
			attrs = _fs->topTarget();
		}
		Expect(_SC('{'));
		if(attrs != -1) _fs->popTarget();
		if(base != -1) _fs->popTarget();
		_fs->addInstruction(_OP_NEWOBJ, _fs->pushTarget(), base, attrs,NOT_CLASS);
		ParseTableOrClass(_SC(';'),_SC('}'));
	}
	void DeleteExpr()
	{
		SQExpState es;
		Lex();
		es = _es;
		_es.donot_get = true;
		PrefixedExpr();
		if(_es.etype==EXPR) error(_SC("can't delete an expression"));
		if(_es.etype==OBJECT || _es.etype==BASE) {
			Emit2ArgsOP(_OP_DELETE);
		}
		else {
			error(_SC("cannot delete an (outer) local"));
		}
		_es = es;
	}
	void PrefixIncDec(int64_t token)
	{
		SQExpState  es;
		int64_t diff = (token==TK_MINUSMINUS) ? -1 : 1;
		Lex();
		es = _es;
		_es.donot_get = true;
		PrefixedExpr();
		if(_es.etype==EXPR) {
			error(_SC("can't '++' or '--' an expression"));
		}
		else if(_es.etype==OBJECT || _es.etype==BASE) {
			Emit2ArgsOP(_OP_INC, diff);
		}
		else if(_es.etype==LOCAL) {
			int64_t src = _fs->topTarget();
			_fs->addInstruction(_OP_INCL, src, src, 0, diff);

		}
		else if(_es.etype==OUTER) {
			int64_t tmp = _fs->pushTarget();
			_fs->addInstruction(_OP_GETOUTER, tmp, _es.epos);
			_fs->addInstruction(_OP_INCL,	 tmp, tmp, 0, diff);
			_fs->addInstruction(_OP_SETOUTER, tmp, _es.epos, tmp);
		}
		_es = es;
	}
	void createFunction(rabbit::Object &name,bool lambda = false)
	{
		SQFuncState *funcstate = _fs->pushChildState(_get_shared_state(_vm));
		funcstate->_name = name;
		rabbit::Object paramname;
		funcstate->addParameter(_fs->createString(_SC("this")));
		funcstate->_sourcename = _sourcename;
		int64_t defparams = 0;
		while(_token!=_SC(')')) {
			if(_token == TK_VARPARAMS) {
				if(defparams > 0) error(_SC("function with default parameters cannot have variable number of parameters"));
				funcstate->addParameter(_fs->createString(_SC("vargv")));
				funcstate->_varparams = true;
				Lex();
				if(_token != _SC(')')) error(_SC("expected ')'"));
				break;
			}
			else {
				paramname = Expect(TK_IDENTIFIER);
				funcstate->addParameter(paramname);
				if(_token == _SC('=')) {
					Lex();
					Expression();
					funcstate->addDefaultParam(_fs->topTarget());
					defparams++;
				}
				else {
					if(defparams > 0) error(_SC("expected '='"));
				}
				if(_token == _SC(',')) Lex();
				else if(_token != _SC(')')) error(_SC("expected ')' or ','"));
			}
		}
		Expect(_SC(')'));
		for(int64_t n = 0; n < defparams; n++) {
			_fs->popTarget();
		}

		SQFuncState *currchunk = _fs;
		_fs = funcstate;
		if(lambda) {
			Expression();
			_fs->addInstruction(_OP_RETURN, 1, _fs->popTarget());}
		else {
			Statement(false);
		}
		funcstate->addLineInfos(_lex._prevtoken == _SC('\n')?_lex._lasttokenline:_lex._currentline, _lineinfo, true);
		funcstate->addInstruction(_OP_RETURN, -1);
		funcstate->setStacksize(0);

		SQFunctionProto *func = funcstate->buildProto();
#ifdef _DEBUG_DUMP
		funcstate->dump(func);
#endif
		_fs = currchunk;
		_fs->_functions.pushBack(func);
		_fs->popChildState();
	}
	void ResolveBreaks(SQFuncState *funcstate, int64_t ntoresolve)
	{
		while(ntoresolve > 0) {
			int64_t pos = funcstate->_unresolvedbreaks.back();
			funcstate->_unresolvedbreaks.popBack();
			//set the jmp instruction
			funcstate->setIntructionParams(pos, 0, funcstate->getCurrentPos() - pos, 0);
			ntoresolve--;
		}
	}
	void ResolveContinues(SQFuncState *funcstate, int64_t ntoresolve, int64_t targetpos)
	{
		while(ntoresolve > 0) {
			int64_t pos = funcstate->_unresolvedcontinues.back();
			funcstate->_unresolvedcontinues.popBack();
			//set the jmp instruction
			funcstate->setIntructionParams(pos, 0, targetpos - pos, 0);
			ntoresolve--;
		}
	}
private:
	int64_t _token;
	SQFuncState *_fs;
	rabbit::ObjectPtr _sourcename;
	SQLexer _lex;
	bool _lineinfo;
	bool _raiseerror;
	int64_t _debugline;
	int64_t _debugop;
	SQExpState   _es;
	SQScope _scope;
	rabbit::Char _compilererror[MAX_COMPILER_ERROR_LEN];
	jmp_buf _errorjmp;
	rabbit::VirtualMachine *_vm;
};

bool compile(rabbit::VirtualMachine *vm,SQLEXREADFUNC rg, rabbit::UserPointer up, const rabbit::Char *sourcename, rabbit::ObjectPtr &out, bool raiseerror, bool lineinfo)
{
	SQcompiler p(vm, rg, up, sourcename, raiseerror, lineinfo);
	return p.compile(out);
}

#endif
