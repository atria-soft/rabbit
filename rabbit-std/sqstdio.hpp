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
	virtual int64_t Read(void *buffer, int64_t size) = 0;
	virtual int64_t Write(void *buffer, int64_t size) = 0;
	virtual int64_t Flush() = 0;
	virtual int64_t Tell() = 0;
	virtual int64_t Len() = 0;
	virtual int64_t Seek(int64_t offset, int64_t origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

#define SQ_SEEK_CUR 0
#define SQ_SEEK_END 1
#define SQ_SEEK_SET 2

typedef void* SQFILE;

RABBIT_API SQFILE sqstd_fopen(const rabbit::Char *,const rabbit::Char *);
RABBIT_API int64_t sqstd_fread(rabbit::UserPointer, int64_t, int64_t, SQFILE);
RABBIT_API int64_t sqstd_fwrite(const rabbit::UserPointer, int64_t, int64_t, SQFILE);
RABBIT_API int64_t sqstd_fseek(SQFILE , int64_t , int64_t);
RABBIT_API int64_t sqstd_ftell(SQFILE);
RABBIT_API int64_t sqstd_fflush(SQFILE);
RABBIT_API int64_t sqstd_fclose(SQFILE);
RABBIT_API int64_t sqstd_feof(SQFILE);

RABBIT_API rabbit::Result sqstd_createfile(rabbit::VirtualMachine* v, SQFILE file,rabbit::Bool own);
RABBIT_API rabbit::Result sqstd_getfile(rabbit::VirtualMachine* v, int64_t idx, SQFILE *file);

//compiler helpers
RABBIT_API rabbit::Result sqstd_loadfile(rabbit::VirtualMachine* v,const rabbit::Char *filename,rabbit::Bool printerror);
RABBIT_API rabbit::Result sqstd_dofile(rabbit::VirtualMachine* v,const rabbit::Char *filename,rabbit::Bool retval,rabbit::Bool printerror);
RABBIT_API rabbit::Result sqstd_writeclosuretofile(rabbit::VirtualMachine* v,const rabbit::Char *filename);

RABBIT_API rabbit::Result sqstd_register_iolib(rabbit::VirtualMachine* v);

