/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

int64_t _stream_readblob(rabbit::VirtualMachine* v);
int64_t _stream_readline(rabbit::VirtualMachine* v);
int64_t _stream_readn(rabbit::VirtualMachine* v);
int64_t _stream_writeblob(rabbit::VirtualMachine* v);
int64_t _stream_writen(rabbit::VirtualMachine* v);
int64_t _stream_seek(rabbit::VirtualMachine* v);
int64_t _stream_tell(rabbit::VirtualMachine* v);
int64_t _stream_len(rabbit::VirtualMachine* v);
int64_t _stream_eos(rabbit::VirtualMachine* v);
int64_t _stream_flush(rabbit::VirtualMachine* v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}
SQRESULT declare_stream(rabbit::VirtualMachine* v,const SQChar* name,SQUserPointer typetag,const SQChar* reg_name,const SQRegFunction *methods,const SQRegFunction *globals);
