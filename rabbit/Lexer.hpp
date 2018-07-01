/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/Vector.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/sqconfig.hpp>
#include <rabbit/Compiler.hpp>

namespace rabbit {
	
	typedef unsigned char Lexchar;
	
	class Lexer {
		public:
			Lexer();
			~Lexer();
			void init(SharedState *ss,SQLEXREADFUNC rg,UserPointer up,compilererrorFunc efunc,void *ed);
			void error(const char *err);
			int64_t Lex();
			const char *tok2Str(int64_t tok);
		private:
			int64_t getIDType(const char *s,int64_t len);
			int64_t readString(int64_t ndelim,bool verbatim);
			int64_t readNumber();
			void lexBlockComment();
			void lexLineComment();
			int64_t readId();
			void next();
			int64_t addUTF8(uint64_t ch);
			int64_t processStringHexEscape(char *dest, int64_t maxdigits);
			int64_t _curtoken;
			Table *_keywords;
			Bool _reached_eof;
		public:
			int64_t _prevtoken;
			int64_t _currentline;
			int64_t _lasttokenline;
			int64_t _currentcolumn;
			const char *_svalue;
			int64_t _nvalue;
			float_t _fvalue;
			SQLEXREADFUNC _readf;
			UserPointer _up;
			Lexchar _currdata;
			SharedState *_sharedstate;
			etk::Vector<char> _longstr;
			compilererrorFunc _errfunc;
			void *_errtarget;
	};
}
