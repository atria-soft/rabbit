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
			const rabbit::Char *begin;
			int64_t len;
		} SQRexMatch;
		
		SQRex *rex_compile(const rabbit::Char *pattern,const rabbit::Char **error);
		void rex_free(SQRex *exp);
		rabbit::Bool rex_match(SQRex* exp,const rabbit::Char* text);
		rabbit::Bool rex_search(SQRex* exp,const rabbit::Char* text, const rabbit::Char** out_begin, const rabbit::Char** out_end);
		rabbit::Bool rex_searchrange(SQRex* exp,const rabbit::Char* text_begin,const rabbit::Char* text_end,const rabbit::Char** out_begin, const rabbit::Char** out_end);
		int64_t rex_getsubexpcount(SQRex* exp);
		rabbit::Bool rex_getsubexp(SQRex* exp, int64_t n, SQRexMatch *subexp);
		
		rabbit::Result format(rabbit::VirtualMachine* v,int64_t nformatstringidx,int64_t *outlen,rabbit::Char **output);
		
		rabbit::Result register_stringlib(rabbit::VirtualMachine* v);

	}
}
