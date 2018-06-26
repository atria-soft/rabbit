/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

SQInteger _stream_readblob(HSQUIRRELVM v);
SQInteger _stream_readline(HSQUIRRELVM v);
SQInteger _stream_readn(HSQUIRRELVM v);
SQInteger _stream_writeblob(HSQUIRRELVM v);
SQInteger _stream_writen(HSQUIRRELVM v);
SQInteger _stream_seek(HSQUIRRELVM v);
SQInteger _stream_tell(HSQUIRRELVM v);
SQInteger _stream_len(HSQUIRRELVM v);
SQInteger _stream_eos(HSQUIRRELVM v);
SQInteger _stream_flush(HSQUIRRELVM v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
SQRESULT declare_stream(HSQUIRRELVM v,const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,const SQRegFunction *methods,const SQRegFunction *globals);
