/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

typedef unsigned int SQRexBool;
typedef struct SQRex SQRex;

typedef struct {
	const rabbit::Char *begin;
	int64_t len;
} SQRexMatch;

RABBIT_API SQRex *sqstd_rex_compile(const rabbit::Char *pattern,const rabbit::Char **error);
RABBIT_API void sqstd_rex_free(SQRex *exp);
RABBIT_API rabbit::Bool sqstd_rex_match(SQRex* exp,const rabbit::Char* text);
RABBIT_API rabbit::Bool sqstd_rex_search(SQRex* exp,const rabbit::Char* text, const rabbit::Char** out_begin, const rabbit::Char** out_end);
RABBIT_API rabbit::Bool sqstd_rex_searchrange(SQRex* exp,const rabbit::Char* text_begin,const rabbit::Char* text_end,const rabbit::Char** out_begin, const rabbit::Char** out_end);
RABBIT_API int64_t sqstd_rex_getsubexpcount(SQRex* exp);
RABBIT_API rabbit::Bool sqstd_rex_getsubexp(SQRex* exp, int64_t n, SQRexMatch *subexp);

RABBIT_API rabbit::Result sqstd_format(rabbit::VirtualMachine* v,int64_t nformatstringidx,int64_t *outlen,rabbit::Char **output);

RABBIT_API rabbit::Result sqstd_register_stringlib(rabbit::VirtualMachine* v);

