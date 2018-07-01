/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/RegFunction.hpp>

namespace rabbit {
	namespace std {
		using SQRexBool = unsigned int;
		using SQRex = struct SQRex;
		
		typedef struct {
			const char *begin;
			int64_t len;
		} SQRexMatch;
		
		SQRex *rex_compile(const char *pattern,const char **error);
		void rex_free(SQRex *exp);
		rabbit::Bool rex_match(SQRex* exp,const char* text);
		rabbit::Bool rex_search(SQRex* exp,const char* text, const char** out_begin, const char** out_end);
		rabbit::Bool rex_searchrange(SQRex* exp,const char* text_begin,const char* text_end,const char** out_begin, const char** out_end);
		int64_t rex_getsubexpcount(SQRex* exp);
		rabbit::Bool rex_getsubexp(SQRex* exp, int64_t n, SQRexMatch *subexp);
		
		rabbit::Result format(rabbit::VirtualMachine* v,int64_t nformatstringidx,int64_t *outlen,char **output);
		
		rabbit::Result register_stringlib(rabbit::VirtualMachine* v);

	}
}
