/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

SQInteger _stream_readblob(HRABBITVM v);
SQInteger _stream_readline(HRABBITVM v);
SQInteger _stream_readn(HRABBITVM v);
SQInteger _stream_writeblob(HRABBITVM v);
SQInteger _stream_writen(HRABBITVM v);
SQInteger _stream_seek(HRABBITVM v);
SQInteger _stream_tell(HRABBITVM v);
SQInteger _stream_len(HRABBITVM v);
SQInteger _stream_eos(HRABBITVM v);
SQInteger _stream_flush(HRABBITVM v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
SQRESULT declare_stream(HRABBITVM v,const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,const SQRegFunction *methods,const SQRegFunction *globals);
