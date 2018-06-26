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
    const SQChar *begin;
    SQInteger len;
} SQRexMatch;

RABBIT_API SQRex *sqstd_rex_compile(const SQChar *pattern,const SQChar **error);
RABBIT_API void sqstd_rex_free(SQRex *exp);
RABBIT_API SQBool sqstd_rex_match(SQRex* exp,const SQChar* text);
RABBIT_API SQBool sqstd_rex_search(SQRex* exp,const SQChar* text, const SQChar** out_begin, const SQChar** out_end);
RABBIT_API SQBool sqstd_rex_searchrange(SQRex* exp,const SQChar* text_begin,const SQChar* text_end,const SQChar** out_begin, const SQChar** out_end);
RABBIT_API SQInteger sqstd_rex_getsubexpcount(SQRex* exp);
RABBIT_API SQBool sqstd_rex_getsubexp(SQRex* exp, SQInteger n, SQRexMatch *subexp);

RABBIT_API SQRESULT sqstd_format(HRABBITVM v,SQInteger nformatstringidx,SQInteger *outlen,SQChar **output);

RABBIT_API SQRESULT sqstd_register_stringlib(HRABBITVM v);

