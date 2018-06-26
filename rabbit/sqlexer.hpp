/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#ifdef SQUNICODE
typedef SQChar LexChar;
#else
typedef unsigned char LexChar;
#endif

struct SQLexer
{
	SQLexer();
	~SQLexer();
	void Init(SQSharedState *ss,SQLEXREADFUNC rg,SQUserPointer up,CompilerErrorFunc efunc,void *ed);
	void Error(const SQChar *err);
	int64_t Lex();
	const SQChar *Tok2Str(int64_t tok);
private:
	int64_t GetIDType(const SQChar *s,int64_t len);
	int64_t ReadString(int64_t ndelim,bool verbatim);
	int64_t ReadNumber();
	void LexBlockComment();
	void LexLineComment();
	int64_t ReadID();
	void Next();
#ifdef SQUNICODE
#if WCHAR_SIZE == 2
	int64_t AddUTF16(uint64_t ch);
#endif
#else
	int64_t AddUTF8(uint64_t ch);
#endif
	int64_t ProcessStringHexEscape(SQChar *dest, int64_t maxdigits);
	int64_t _curtoken;
	SQTable *_keywords;
	SQBool _reached_eof;
public:
	int64_t _prevtoken;
	int64_t _currentline;
	int64_t _lasttokenline;
	int64_t _currentcolumn;
	const SQChar *_svalue;
	int64_t _nvalue;
	float_t _fvalue;
	SQLEXREADFUNC _readf;
	SQUserPointer _up;
	LexChar _currdata;
	SQSharedState *_sharedstate;
	sqvector<SQChar> _longstr;
	CompilerErrorFunc _errfunc;
	void *_errtarget;
};

