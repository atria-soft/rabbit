/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	
	#ifdef SQUNICODE
	typedef Char LexChar;
	#else
	typedef unsigned char LexChar;
	#endif
	
	class Lexer {
		public:
			Lexer();
			~Lexer();
			void init(SharedState *ss,SQLEXREADFUNC rg,UserPointer up,compilererrorFunc efunc,void *ed);
			void error(const Char *err);
			int64_t Lex();
			const Char *tok2Str(int64_t tok);
		private:
			int64_t getIDType(const Char *s,int64_t len);
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
			int64_t processStringHexEscape(Char *dest, int64_t maxdigits);
			int64_t _curtoken;
			Table *_keywords;
			Bool _reached_eof;
		public:
			int64_t _prevtoken;
			int64_t _currentline;
			int64_t _lasttokenline;
			int64_t _currentcolumn;
			const Char *_svalue;
			int64_t _nvalue;
			float_t _fvalue;
			SQLEXREADFUNC _readf;
			UserPointer _up;
			LexChar _currdata;
			SharedState *_sharedstate;
			etk::Vector<Char> _longstr;
			compilererrorFunc _errfunc;
			void *_errtarget;
	};
}
