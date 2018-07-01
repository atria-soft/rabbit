/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	namespace std {
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

using SQFILE = void*;

SQFILE fopen(const rabbit::Char *,const rabbit::Char *);
int64_t fread(rabbit::UserPointer, int64_t, int64_t, SQFILE);
int64_t fwrite(const rabbit::UserPointer, int64_t, int64_t, SQFILE);
int64_t fseek(SQFILE , int64_t , int64_t);
int64_t ftell(SQFILE);
int64_t fflush(SQFILE);
int64_t fclose(SQFILE);
int64_t feof(SQFILE);

rabbit::Result createfile(rabbit::VirtualMachine* v, SQFILE file,rabbit::Bool own);
rabbit::Result getfile(rabbit::VirtualMachine* v, int64_t idx, SQFILE *file);

//compiler helpers
rabbit::Result loadfile(rabbit::VirtualMachine* v,const rabbit::Char *filename,rabbit::Bool printerror);
rabbit::Result dofile(rabbit::VirtualMachine* v,const rabbit::Char *filename,rabbit::Bool retval,rabbit::Bool printerror);
rabbit::Result writeclosuretofile(rabbit::VirtualMachine* v,const rabbit::Char *filename);

rabbit::Result register_iolib(rabbit::VirtualMachine* v);

	}
}
