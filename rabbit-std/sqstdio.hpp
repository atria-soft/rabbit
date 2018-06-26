/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#define SQSTD_STREAM_TYPE_TAG 0x80000000

struct SQStream {
	virtual ~SQStream() {}
	virtual SQInteger Read(void *buffer, SQInteger size) = 0;
	virtual SQInteger Write(void *buffer, SQInteger size) = 0;
	virtual SQInteger Flush() = 0;
	virtual SQInteger Tell() = 0;
	virtual SQInteger Len() = 0;
	virtual SQInteger Seek(SQInteger offset, SQInteger origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

typedef void* SQFILE;

RABBIT_API SQFILE sqstd_fopen(const SQChar *,const SQChar *);
RABBIT_API SQInteger sqstd_fread(SQUserPointer, SQInteger, SQInteger, SQFILE);
RABBIT_API SQInteger sqstd_fwrite(const SQUserPointer, SQInteger, SQInteger, SQFILE);
RABBIT_API SQInteger sqstd_fseek(SQFILE , SQInteger , SQInteger);
RABBIT_API SQInteger sqstd_ftell(SQFILE);
RABBIT_API SQInteger sqstd_fflush(SQFILE);
RABBIT_API SQInteger sqstd_fclose(SQFILE);
RABBIT_API SQInteger sqstd_feof(SQFILE);

RABBIT_API SQRESULT sqstd_createfile(HRABBITVM v, SQFILE file,SQBool own);
RABBIT_API SQRESULT sqstd_getfile(HRABBITVM v, SQInteger idx, SQFILE *file);

//compiler helpers
RABBIT_API SQRESULT sqstd_loadfile(HRABBITVM v,const SQChar *filename,SQBool printerror);
RABBIT_API SQRESULT sqstd_dofile(HRABBITVM v,const SQChar *filename,SQBool retval,SQBool printerror);
RABBIT_API SQRESULT sqstd_writeclosuretofile(HRABBITVM v,const SQChar *filename);

RABBIT_API SQRESULT sqstd_register_iolib(HRABBITVM v);

