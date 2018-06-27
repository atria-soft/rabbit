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
	void init(SQSharedState *ss,SQLEXREADFUNC rg,SQUserPointer up,compilererrorFunc efunc,void *ed);
	void error(const SQChar *err);
	int64_t Lex();
	const SQChar *tok2Str(int64_t tok);
private:
	int64_t getIDType(const SQChar *s,int64_t len);
	int64_t readString(int64_t ndelim,bool verbatim);
	int64_t readNumber();
	void lexBlockComment();
	void lexLineComment();
	int64_t readId();
	void next();
#ifdef SQUNICODE
#if WCHAR_SIZE == 2
	int64_t addUTF16(uint64_t ch);
#endif
#else
	int64_t addUTF8(uint64_t ch);
#endif
	int64_t processStringHexEscape(SQChar *dest, int64_t maxdigits);
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
	compilererrorFunc _errfunc;
	void *_errtarget;
};

