/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Lexer.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/String.hpp>
#include <rabbit/sqconfig.hpp>


#define CUR_CHAR (_currdata)
#define RETURN_TOKEN(t) { _prevtoken = _curtoken; _curtoken = t; return t;}
#define IS_EOB() (CUR_CHAR <= RABBIT_EOB)
#define NEXT() {next();_currentcolumn++;}
#define INIT_TEMP_STRING() { _longstr.resize(0);}
#define APPEND_CHAR(c) { _longstr.pushBack(c);}
#define TERMINATE_BUFFER() {_longstr.pushBack('\0');}
#define ADD_KEYWORD(key,id) _keywords->newSlot( rabbit::String::create(ss, #key) ,int64_t(id))

rabbit::Lexer::Lexer(){}
rabbit::Lexer::~Lexer()
{
	_keywords->release();
}

void rabbit::Lexer::init(rabbit::SharedState *ss, SQLEXREADFUNC rg, rabbit::UserPointer up,compilererrorFunc efunc,void *ed)
{
	_errfunc = efunc;
	_errtarget = ed;
	_sharedstate = ss;
	_keywords = rabbit::Table::create(ss, 37);
	ADD_KEYWORD(while, TK_WHILE);
	ADD_KEYWORD(do, TK_DO);
	ADD_KEYWORD(if, TK_IF);
	ADD_KEYWORD(else, TK_ELSE);
	ADD_KEYWORD(break, TK_BREAK);
	ADD_KEYWORD(continue, TK_CONTINUE);
	ADD_KEYWORD(return, TK_RETURN);
	ADD_KEYWORD(null, TK_NULL);
	ADD_KEYWORD(function, TK_FUNCTION);
	ADD_KEYWORD(local, TK_LOCAL);
	ADD_KEYWORD(for, TK_FOR);
	ADD_KEYWORD(foreach, TK_FOREACH);
	ADD_KEYWORD(in, TK_IN);
	ADD_KEYWORD(typeof, TK_TYPEOF);
	ADD_KEYWORD(base, TK_BASE);
	ADD_KEYWORD(delete, TK_DELETE);
	ADD_KEYWORD(try, TK_TRY);
	ADD_KEYWORD(catch, TK_CATCH);
	ADD_KEYWORD(throw, TK_THROW);
	ADD_KEYWORD(clone, TK_CLONE);
	ADD_KEYWORD(yield, TK_YIELD);
	ADD_KEYWORD(resume, TK_RESUME);
	ADD_KEYWORD(switch, TK_SWITCH);
	ADD_KEYWORD(case, TK_CASE);
	ADD_KEYWORD(default, TK_DEFAULT);
	ADD_KEYWORD(this, TK_THIS);
	ADD_KEYWORD(class,TK_CLASS);
	ADD_KEYWORD(extends,TK_EXTENDS);
	ADD_KEYWORD(constructor,TK_CONSTRUCTOR);
	ADD_KEYWORD(instanceof,TK_INSTANCEOF);
	ADD_KEYWORD(true,TK_TRUE);
	ADD_KEYWORD(false,TK_FALSE);
	ADD_KEYWORD(static,TK_STATIC);
	ADD_KEYWORD(enum,TK_ENUM);
	ADD_KEYWORD(const,TK_CONST);
	ADD_KEYWORD(__LINE__,TK___LINE__);
	ADD_KEYWORD(__FILE__,TK___FILE__);
	ADD_KEYWORD(rawcall, TK_RAWCALL);


	_readf = rg;
	_up = up;
	_lasttokenline = _currentline = 1;
	_currentcolumn = 0;
	_prevtoken = -1;
	_reached_eof = SQFalse;
	next();
}

void rabbit::Lexer::error(const char *err)
{
	_errfunc(_errtarget,err);
}

void rabbit::Lexer::next()
{
	int64_t t = _readf(_up);
	if(t > UINT8_MAX) error("Invalid character");
	if(t != 0) {
		_currdata = (Lexchar)t;
		return;
	}
	_currdata = RABBIT_EOB;
	_reached_eof = SQTrue;
}

const char *rabbit::Lexer::tok2Str(int64_t tok)
{
	rabbit::ObjectPtr itr, key, val;
	int64_t nitr;
	while((nitr = _keywords->next(false,itr, key, val)) != -1) {
		itr = (int64_t)nitr;
		if(((int64_t)val.toInteger()) == tok)
			return _stringval(key);
	}
	return NULL;
}

void rabbit::Lexer::lexBlockComment()
{
	bool done = false;
	while(!done) {
		switch(CUR_CHAR) {
			case '*': { NEXT(); if(CUR_CHAR == '/') { done = true; NEXT(); }}; continue;
			case '\n': _currentline++; NEXT(); continue;
			case RABBIT_EOB: error("missing \"*/\" in comment");
			default: NEXT();
		}
	}
}
void rabbit::Lexer::lexLineComment()
{
	do { NEXT(); } while (CUR_CHAR != '\n' && (!IS_EOB()));
}

int64_t rabbit::Lexer::Lex()
{
	_lasttokenline = _currentline;
	while(CUR_CHAR != RABBIT_EOB) {
		switch(CUR_CHAR){
		case '\t': case '\r': case ' ': NEXT(); continue;
		case '\n':
			_currentline++;
			_prevtoken=_curtoken;
			_curtoken='\n';
			NEXT();
			_currentcolumn=1;
			continue;
		case '#': lexLineComment(); continue;
		case '/':
			NEXT();
			switch(CUR_CHAR){
			case '*':
				NEXT();
				lexBlockComment();
				continue;
			case '/':
				lexLineComment();
				continue;
			case '=':
				NEXT();
				RETURN_TOKEN(TK_DIVEQ);
				continue;
			case '>':
				NEXT();
				RETURN_TOKEN(TK_ATTR_CLOSE);
				continue;
			default:
				RETURN_TOKEN('/');
			}
		case '=':
			NEXT();
			if (CUR_CHAR != '='){ RETURN_TOKEN('=') }
			else { NEXT(); RETURN_TOKEN(TK_EQ); }
		case '<':
			NEXT();
			switch(CUR_CHAR) {
			case '=':
				NEXT();
				if(CUR_CHAR == '>') {
					NEXT();
					RETURN_TOKEN(TK_3WAYSCMP);
				}
				RETURN_TOKEN(TK_LE)
				break;
			case '-': NEXT(); RETURN_TOKEN(TK_NEWSLOT); break;
			case '<': NEXT(); RETURN_TOKEN(TK_SHIFTL); break;
			case '/': NEXT(); RETURN_TOKEN(TK_ATTR_OPEN); break;
			}
			RETURN_TOKEN('<');
		case '>':
			NEXT();
			if (CUR_CHAR == '='){ NEXT(); RETURN_TOKEN(TK_GE);}
			else if(CUR_CHAR == '>'){
				NEXT();
				if(CUR_CHAR == '>'){
					NEXT();
					RETURN_TOKEN(TK_USHIFTR);
				}
				RETURN_TOKEN(TK_SHIFTR);
			}
			else { RETURN_TOKEN('>') }
		case '!':
			NEXT();
			if (CUR_CHAR != '='){ RETURN_TOKEN('!')}
			else { NEXT(); RETURN_TOKEN(TK_NE); }
		case '@': {
			int64_t stype;
			NEXT();
			if(CUR_CHAR != '"') {
				RETURN_TOKEN('@');
			}
			if((stype=readString('"',true))!=-1) {
				RETURN_TOKEN(stype);
			}
			error("error parsing the string");
					   }
		case '"':
		case '\'': {
			int64_t stype;
			if((stype=readString(CUR_CHAR,false))!=-1){
				RETURN_TOKEN(stype);
			}
			error("error parsing the string");
			}
		case '{': case '}': case '(': case ')': case '[': case ']':
		case ';': case ',': case '?': case '^': case '~':
			{int64_t ret = CUR_CHAR;
			NEXT(); RETURN_TOKEN(ret); }
		case '.':
			NEXT();
			if (CUR_CHAR != '.'){ RETURN_TOKEN('.') }
			NEXT();
			if (CUR_CHAR != '.'){ error("invalid token '..'"); }
			NEXT();
			RETURN_TOKEN(TK_VARPARAMS);
		case '&':
			NEXT();
			if (CUR_CHAR != '&'){ RETURN_TOKEN('&') }
			else { NEXT(); RETURN_TOKEN(TK_AND); }
		case '|':
			NEXT();
			if (CUR_CHAR != '|'){ RETURN_TOKEN('|') }
			else { NEXT(); RETURN_TOKEN(TK_OR); }
		case ':':
			NEXT();
			if (CUR_CHAR != ':'){ RETURN_TOKEN(':') }
			else { NEXT(); RETURN_TOKEN(TK_DOUBLE_COLON); }
		case '*':
			NEXT();
			if (CUR_CHAR == '='){ NEXT(); RETURN_TOKEN(TK_MULEQ);}
			else RETURN_TOKEN('*');
		case '%':
			NEXT();
			if (CUR_CHAR == '='){ NEXT(); RETURN_TOKEN(TK_MODEQ);}
			else RETURN_TOKEN('%');
		case '-':
			NEXT();
			if (CUR_CHAR == '='){ NEXT(); RETURN_TOKEN(TK_MINUSEQ);}
			else if  (CUR_CHAR == '-'){ NEXT(); RETURN_TOKEN(TK_MINUSMINUS);}
			else RETURN_TOKEN('-');
		case '+':
			NEXT();
			if (CUR_CHAR == '='){ NEXT(); RETURN_TOKEN(TK_PLUSEQ);}
			else if (CUR_CHAR == '+'){ NEXT(); RETURN_TOKEN(TK_PLUSPLUS);}
			else RETURN_TOKEN('+');
		case RABBIT_EOB:
			return 0;
		default:{
				if (isdigit(CUR_CHAR)) {
					int64_t ret = readNumber();
					RETURN_TOKEN(ret);
				}
				else if (isalpha(CUR_CHAR) || CUR_CHAR == '_') {
					int64_t t = readId();
					RETURN_TOKEN(t);
				}
				else {
					int64_t c = CUR_CHAR;
					if (iscntrl((int)c)) error("unexpected character(control)");
					NEXT();
					RETURN_TOKEN(c);
				}
				RETURN_TOKEN(0);
			}
		}
	}
	return 0;
}

int64_t rabbit::Lexer::getIDType(const char *s,int64_t len)
{
	rabbit::ObjectPtr t;
	if(_keywords->getStr(s,len, t)) {
		return int64_t(t.toInteger());
	}
	return TK_IDENTIFIER;
}

int64_t rabbit::Lexer::addUTF8(uint64_t ch)
{
	if (ch < 0x80) {
		APPEND_CHAR((char)ch);
		return 1;
	}
	if (ch < 0x800) {
		APPEND_CHAR((char)((ch >> 6) | 0xC0));
		APPEND_CHAR((char)((ch & 0x3F) | 0x80));
		return 2;
	}
	if (ch < 0x10000) {
		APPEND_CHAR((char)((ch >> 12) | 0xE0));
		APPEND_CHAR((char)(((ch >> 6) & 0x3F) | 0x80));
		APPEND_CHAR((char)((ch & 0x3F) | 0x80));
		return 3;
	}
	if (ch < 0x110000) {
		APPEND_CHAR((char)((ch >> 18) | 0xF0));
		APPEND_CHAR((char)(((ch >> 12) & 0x3F) | 0x80));
		APPEND_CHAR((char)(((ch >> 6) & 0x3F) | 0x80));
		APPEND_CHAR((char)((ch & 0x3F) | 0x80));
		return 4;
	}
	return 0;
}

int64_t rabbit::Lexer::processStringHexEscape(char *dest, int64_t maxdigits)
{
	NEXT();
	if (!isxdigit(CUR_CHAR)) error("hexadecimal number expected");
	int64_t n = 0;
	while (isxdigit(CUR_CHAR) && n < maxdigits) {
		dest[n] = CUR_CHAR;
		n++;
		NEXT();
	}
	dest[n] = 0;
	return n;
}

int64_t rabbit::Lexer::readString(int64_t ndelim,bool verbatim)
{
	INIT_TEMP_STRING();
	NEXT();
	if(IS_EOB()) return -1;
	for(;;) {
		while(CUR_CHAR != ndelim) {
			int64_t x = CUR_CHAR;
			switch (x) {
			case RABBIT_EOB:
				error("unfinished string");
				return -1;
			case '\n':
				if(!verbatim) error("newline in a constant");
				APPEND_CHAR(CUR_CHAR); NEXT();
				_currentline++;
				break;
			case '\\':
				if(verbatim) {
					APPEND_CHAR('\\'); NEXT();
				}
				else {
					NEXT();
					switch(CUR_CHAR) {
					case 'x':  {
						const int64_t maxdigits = sizeof(char) * 2;
						char temp[maxdigits + 1];
						processStringHexEscape(temp, maxdigits);
						char *stemp;
						APPEND_CHAR((char)strtoul(temp, &stemp, 16));
					}
					break;
					case 'U':
					case 'u':  {
						const int64_t maxdigits = CUR_CHAR == 'u' ? 4 : 8;
						char temp[8 + 1];
						processStringHexEscape(temp, maxdigits);
						char *stemp;
						addUTF8(strtoul(temp, &stemp, 16));
					}
					break;
					case 't': APPEND_CHAR('\t'); NEXT(); break;
					case 'a': APPEND_CHAR('\a'); NEXT(); break;
					case 'b': APPEND_CHAR('\b'); NEXT(); break;
					case 'n': APPEND_CHAR('\n'); NEXT(); break;
					case 'r': APPEND_CHAR('\r'); NEXT(); break;
					case 'v': APPEND_CHAR('\v'); NEXT(); break;
					case 'f': APPEND_CHAR('\f'); NEXT(); break;
					case '0': APPEND_CHAR('\0'); NEXT(); break;
					case '\\': APPEND_CHAR('\\'); NEXT(); break;
					case '"': APPEND_CHAR('"'); NEXT(); break;
					case '\'': APPEND_CHAR('\''); NEXT(); break;
					default:
						error("unrecognised escaper char");
					break;
					}
				}
				break;
			default:
				APPEND_CHAR(CUR_CHAR);
				NEXT();
			}
		}
		NEXT();
		if(verbatim && CUR_CHAR == '"') { //double quotation
			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
		else {
			break;
		}
	}
	TERMINATE_BUFFER();
	int64_t len = _longstr.size()-1;
	if(ndelim == '\'') {
		if(len == 0) error("empty constant");
		if(len > 1) error("constant too long");
		_nvalue = _longstr[0];
		return TK_INTEGER;
	}
	_svalue = &_longstr[0];
	return TK_STRING_LITERAL;
}

void LexHexadecimal(const char *s,uint64_t *res)
{
	*res = 0;
	while(*s != 0)
	{
		if(isdigit(*s)) *res = (*res)*16+((*s++)-'0');
		else if(isxdigit(*s)) *res = (*res)*16+(toupper(*s++)-'A'+10);
		else { assert(0); }
	}
}

void LexInteger(const char *s,uint64_t *res)
{
	*res = 0;
	while(*s != 0)
	{
		*res = (*res)*10+((*s++)-'0');
	}
}

int64_t scisodigit(int64_t c) { return c >= '0' && c <= '7'; }

void LexOctal(const char *s,uint64_t *res)
{
	*res = 0;
	while(*s != 0)
	{
		if(scisodigit(*s)) *res = (*res)*8+((*s++)-'0');
		else { assert(0); }
	}
}

int64_t isexponent(int64_t c) { return c == 'e' || c=='E'; }


#define MAX_HEX_DIGITS (sizeof(int64_t)*2)
int64_t rabbit::Lexer::readNumber()
{
#define TINT 1
#define TFLOAT 2
#define THEX 3
#define TSCIENTIFIC 4
#define TOCTAL 5
	int64_t type = TINT, firstchar = CUR_CHAR;
	char *sTemp;
	INIT_TEMP_STRING();
	NEXT();
	if(firstchar == '0' && (toupper(CUR_CHAR) == 'X' || scisodigit(CUR_CHAR)) ) {
		if(scisodigit(CUR_CHAR)) {
			type = TOCTAL;
			while(scisodigit(CUR_CHAR)) {
				APPEND_CHAR(CUR_CHAR);
				NEXT();
			}
			if(isdigit(CUR_CHAR)) error("invalid octal number");
		}
		else {
			NEXT();
			type = THEX;
			while(isxdigit(CUR_CHAR)) {
				APPEND_CHAR(CUR_CHAR);
				NEXT();
			}
			if(_longstr.size() > MAX_HEX_DIGITS) error("too many digits for an Hex number");
		}
	}
	else {
		APPEND_CHAR((int)firstchar);
		while (CUR_CHAR == '.' || isdigit(CUR_CHAR) || isexponent(CUR_CHAR)) {
			if(CUR_CHAR == '.' || isexponent(CUR_CHAR)) type = TFLOAT;
			if(isexponent(CUR_CHAR)) {
				if(type != TFLOAT) error("invalid numeric format");
				type = TSCIENTIFIC;
				APPEND_CHAR(CUR_CHAR);
				NEXT();
				if(CUR_CHAR == '+' || CUR_CHAR == '-'){
					APPEND_CHAR(CUR_CHAR);
					NEXT();
				}
				if(!isdigit(CUR_CHAR)) error("exponent expected");
			}

			APPEND_CHAR(CUR_CHAR);
			NEXT();
		}
	}
	TERMINATE_BUFFER();
	switch(type) {
	case TSCIENTIFIC:
	case TFLOAT:
		_fvalue = (float_t)strtod(&_longstr[0],&sTemp);
		return TK_FLOAT;
	case TINT:
		LexInteger(&_longstr[0],(uint64_t *)&_nvalue);
		return TK_INTEGER;
	case THEX:
		LexHexadecimal(&_longstr[0],(uint64_t *)&_nvalue);
		return TK_INTEGER;
	case TOCTAL:
		LexOctal(&_longstr[0],(uint64_t *)&_nvalue);
		return TK_INTEGER;
	}
	return 0;
}

int64_t rabbit::Lexer::readId()
{
	int64_t res;
	INIT_TEMP_STRING();
	do {
		APPEND_CHAR(CUR_CHAR);
		NEXT();
	} while(isalnum(CUR_CHAR) || CUR_CHAR == '_');
	TERMINATE_BUFFER();
	res = getIDType(&_longstr[0],_longstr.size() - 1);
	if(res == TK_IDENTIFIER || res == TK_CONSTRUCTOR) {
		_svalue = &_longstr[0];
	}
	return res;
}
