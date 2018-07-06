/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <new>
#include <stdio.h>
#include <rabbit/rabbit.hpp>
#include <rabbit-std/sqstdio.hpp>
#include <rabbit-std/sqstdstream.hpp>

#define SQSTD_FILE_TYPE_TAG ((uint64_t)(SQSTD_STREAM_TYPE_TAG | 0x00000001))
//basic API
rabbit::std::SQFILE rabbit::std::fopen(const char *filename ,const char *mode)
{
	return (SQFILE)::fopen(filename,mode);
}

int64_t rabbit::std::fread(void* buffer, int64_t size, int64_t count, SQFILE file)
{
	int64_t ret = (int64_t)::fread(buffer,size,count,(FILE *)file);
	return ret;
}

int64_t rabbit::std::fwrite(const rabbit::UserPointer buffer, int64_t size, int64_t count, SQFILE file)
{
	return (int64_t)::fwrite(buffer,size,count,(FILE *)file);
}

int64_t rabbit::std::fseek(SQFILE file, int64_t offset, int64_t origin)
{
	int64_t realorigin;
	switch(origin) {
		case SQ_SEEK_CUR: realorigin = SEEK_CUR; break;
		case SQ_SEEK_END: realorigin = SEEK_END; break;
		case SQ_SEEK_SET: realorigin = SEEK_SET; break;
		default: return -1; //failed
	}
	return ::fseek((FILE *)file,(long)offset,(int)realorigin);
}

int64_t rabbit::std::ftell(SQFILE file)
{
	return ::ftell((FILE *)file);
}

int64_t rabbit::std::fflush(SQFILE file)
{
	return ::fflush((FILE *)file);
}

int64_t rabbit::std::fclose(SQFILE file)
{
	return ::fclose((FILE *)file);
}

int64_t rabbit::std::feof(SQFILE file)
{
	return ::feof((FILE *)file);
}
namespace rabbit {

	namespace std {
		//File
		struct File : public rabbit::std::SQStream {
			File() { _handle = NULL; _owns = false;}
			File(SQFILE file, bool owns) { _handle = file; _owns = owns;}
			virtual ~File() { close(); }
			bool Open(const char *filename ,const char *mode) {
				close();
				if( (_handle = rabbit::std::fopen(filename,mode)) ) {
					_owns = true;
					return true;
				}
				return false;
			}
			void close() {
				if(_handle && _owns) {
					rabbit::std::fclose(_handle);
					_handle = NULL;
					_owns = false;
				}
			}
			int64_t Read(void *buffer,int64_t size) {
				return rabbit::std::fread(buffer,1,size,_handle);
			}
			int64_t Write(void *buffer,int64_t size) {
				return rabbit::std::fwrite(buffer,1,size,_handle);
			}
			int64_t Flush() {
				return rabbit::std::fflush(_handle);
			}
			int64_t Tell() {
				return rabbit::std::ftell(_handle);
			}
			int64_t Len() {
				int64_t prevpos=Tell();
				Seek(0,SQ_SEEK_END);
				int64_t size=Tell();
				Seek(prevpos,SQ_SEEK_SET);
				return size;
			}
			int64_t Seek(int64_t offset, int64_t origin)  {
				return rabbit::std::fseek(_handle,offset,origin);
			}
			bool IsValid() { return _handle?true:false; }
			bool EOS() { return Tell()==Len()?true:false;}
			SQFILE getHandle() {return _handle;}
		private:
			SQFILE _handle;
			bool _owns;
		};
	}
}
static int64_t _file__typeof(rabbit::VirtualMachine* v)
{
	sq_pushstring(v,"file",-1);
	return 1;
}

static int64_t _file_releasehook(rabbit::UserPointer p, int64_t SQ_UNUSED_ARG(size))
{
	rabbit::std::File *self = (rabbit::std::File*)p;
	self->~File();
	rabbit::sq_free(self,sizeof(rabbit::std::File));
	return 1;
}

static int64_t _file_constructor(rabbit::VirtualMachine* v)
{
	const char *filename,*mode;
	bool owns = true;
	rabbit::std::File *f;
	rabbit::std::SQFILE newf;
	if(rabbit::sq_gettype(v,2) == rabbit::OT_STRING && sq_gettype(v,3) == rabbit::OT_STRING) {
		rabbit::sq_getstring(v, 2, &filename);
		rabbit::sq_getstring(v, 3, &mode);
		newf = rabbit::std::fopen(filename, mode);
		if(!newf) return rabbit::sq_throwerror(v, "cannot open file");
	} else if(rabbit::sq_gettype(v,2) == rabbit::OT_USERPOINTER) {
		owns = !(rabbit::sq_gettype(v,3) == rabbit::OT_NULL);
		rabbit::sq_getuserpointer(v,2,&newf);
	} else {
		return rabbit::sq_throwerror(v,"wrong parameter");
	}

	f = new (rabbit::sq_malloc(sizeof(rabbit::std::File)))rabbit::std::File(newf,owns);
	if(SQ_FAILED(rabbit::sq_setinstanceup(v,1,f))) {
		f->~File();
		rabbit::sq_free(f,sizeof(rabbit::std::File));
		return rabbit::sq_throwerror(v, "cannot create blob with negative size");
	}
	rabbit::sq_setreleasehook(v,1,_file_releasehook);
	return 0;
}

static int64_t _file_close(rabbit::VirtualMachine* v)
{
	rabbit::std::File *self = NULL;
	if(SQ_SUCCEEDED(rabbit::sq_getinstanceup(v,1,(rabbit::UserPointer*)&self,(rabbit::UserPointer)SQSTD_FILE_TYPE_TAG))
		&& self != NULL)
	{
		self->close();
	}
	return 0;
}

//bindings
#define _DECL_FILE_FUNC(name,nparams,typecheck) {#name,_file_##name,nparams,typecheck}
static const rabbit::RegFunction _file_methods[] = {
	_DECL_FILE_FUNC(constructor,3,"x"),
	_DECL_FILE_FUNC(_typeof,1,"x"),
	_DECL_FILE_FUNC(close,1,"x"),
	{NULL,(SQFUNCTION)0,0,NULL}
};



rabbit::Result rabbit::std::createfile(rabbit::VirtualMachine* v, SQFILE file,rabbit::Bool own)
{
	int64_t top = sq_gettop(v);
	sq_pushregistrytable(v);
	sq_pushstring(v,"std_file",-1);
	if(SQ_SUCCEEDED(sq_get(v,-2))) {
		sq_remove(v,-2); //removes the registry
		sq_pushroottable(v); // push the this
		sq_pushuserpointer(v,file); //file
		if(own){
			sq_pushinteger(v,1); //true
		}
		else{
			sq_pushnull(v); //false
		}
		if(SQ_SUCCEEDED( sq_call(v,3,SQTrue,SQFalse) )) {
			sq_remove(v,-2);
			return SQ_OK;
		}
	}
	sq_settop(v,top);
	return SQ_ERROR;
}

rabbit::Result rabbit::std::getfile(rabbit::VirtualMachine* v, int64_t idx, SQFILE *file)
{
	rabbit::std::File *fileobj = NULL;
	if(SQ_SUCCEEDED(sq_getinstanceup(v,idx,(rabbit::UserPointer*)&fileobj,(rabbit::UserPointer)SQSTD_FILE_TYPE_TAG))) {
		*file = fileobj->getHandle();
		return SQ_OK;
	}
	return sq_throwerror(v,"not a file");
}



#define IO_BUFFER_SIZE 2048
struct IOBuffer {
	unsigned char buffer[IO_BUFFER_SIZE];
	int64_t size;
	int64_t ptr;
	rabbit::std::SQFILE file;
};

int64_t _read_byte(IOBuffer *iobuffer)
{
	if(iobuffer->ptr < iobuffer->size) {

		int64_t ret = iobuffer->buffer[iobuffer->ptr];
		iobuffer->ptr++;
		return ret;
	}
	else {
		if( (iobuffer->size = rabbit::std::fread(iobuffer->buffer,1,IO_BUFFER_SIZE,iobuffer->file )) > 0 )
		{
			int64_t ret = iobuffer->buffer[0];
			iobuffer->ptr = 1;
			return ret;
		}
	}

	return 0;
}

int64_t _read_two_bytes(IOBuffer *iobuffer)
{
	if(iobuffer->ptr < iobuffer->size) {
		if(iobuffer->size < 2) return 0;
		int64_t ret = *((const wchar_t*)&iobuffer->buffer[iobuffer->ptr]);
		iobuffer->ptr += 2;
		return ret;
	}
	else {
		if( (iobuffer->size = rabbit::std::fread(iobuffer->buffer,1,IO_BUFFER_SIZE,iobuffer->file )) > 0 )
		{
			if(iobuffer->size < 2) return 0;
			int64_t ret = *((const wchar_t*)&iobuffer->buffer[0]);
			iobuffer->ptr = 2;
			return ret;
		}
	}

	return 0;
}

static int64_t _io_file_lexfeed_PLAIN(rabbit::UserPointer iobuf)
{
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	return _read_byte(iobuffer);

}

static int64_t _io_file_lexfeed_UCS2_LE(rabbit::UserPointer iobuf)
{
	int64_t ret;
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	if( (ret = _read_two_bytes(iobuffer)) > 0 )
		return ret;
	return 0;
}

static int64_t _io_file_lexfeed_UCS2_BE(rabbit::UserPointer iobuf)
{
	int64_t c;
	IOBuffer *iobuffer = (IOBuffer *)iobuf;
	if( (c = _read_two_bytes(iobuffer)) > 0 ) {
		c = ((c>>8)&0x00FF)| ((c<<8)&0xFF00);
		return c;
	}
	return 0;
}

int64_t file_read(rabbit::UserPointer file,rabbit::UserPointer buf,int64_t size)
{
	int64_t ret;
	if( ( ret = rabbit::std::fread(buf,1,size,(rabbit::std::SQFILE)file ))!=0 )return ret;
	return -1;
}

int64_t file_write(rabbit::UserPointer file,rabbit::UserPointer p,int64_t size)
{
	return rabbit::std::fwrite(p,1,size,(rabbit::std::SQFILE)file);
}

rabbit::Result rabbit::std::loadfile(rabbit::VirtualMachine* v,const char *filename,rabbit::Bool printerror)
{
	SQFILE file = rabbit::std::fopen(filename,"rb");

	int64_t ret;
	unsigned short us;
	unsigned char uc;
	SQLEXREADFUNC func = _io_file_lexfeed_PLAIN;
	if(file){
		ret = rabbit::std::fread(&us,1,2,file);
		if(ret != 2) {
			//probably an empty file
			us = 0;
		}
		switch (us) {
			//gotta swap the next 2 lines on BIG endian machines
			case 0xFFFE:
				func = _io_file_lexfeed_UCS2_BE;
				break; //UTF-16 little endian;
			case 0xFEFF:
				func = _io_file_lexfeed_UCS2_LE;
				break; //UTF-16 big endian;
			case 0xBBEF:
				if(rabbit::std::fread(&uc,1,sizeof(uc),file) == 0) {
					rabbit::std::fclose(file);
					return sq_throwerror(v,"io error");
				}
				if(uc != 0xBF) {
					rabbit::std::fclose(file);
					return sq_throwerror(v,"Unrecognized encoding");
				}
				func = _io_file_lexfeed_PLAIN;
				break;
				//UTF-8 ;
			default:
				rabbit::std::fseek(file,0,SQ_SEEK_SET);
				break;
				// ascii
		}
		IOBuffer buffer;
		buffer.ptr = 0;
		buffer.size = 0;
		buffer.file = file;
		if(SQ_SUCCEEDED(sq_compile(v,func,&buffer,filename,printerror))){
			rabbit::std::fclose(file);
			return SQ_OK;
		}
		rabbit::std::fclose(file);
		return SQ_ERROR;
	}
	return sq_throwerror(v,"cannot open the file");
}

rabbit::Result rabbit::std::dofile(rabbit::VirtualMachine* v,const char *filename,rabbit::Bool retval,rabbit::Bool printerror)
{
	//at least one entry must exist in order for us to push it as the environment
	if(sq_gettop(v) == 0)
		return sq_throwerror(v,"environment table expected");

	if(SQ_SUCCEEDED(rabbit::std::loadfile(v,filename,printerror))) {
		sq_push(v,-2);
		if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue))) {
			sq_remove(v,retval?-2:-1); //removes the closure
			return 1;
		}
		sq_pop(v,1); //removes the closure
	}
	return SQ_ERROR;
}

int64_t _g_io_loadfile(rabbit::VirtualMachine* v)
{
	const char *filename;
	rabbit::Bool printerror = SQFalse;
	sq_getstring(v,2,&filename);
	if(sq_gettop(v) >= 3) {
		sq_getbool(v,3,&printerror);
	}
	if(SQ_SUCCEEDED(rabbit::std::loadfile(v,filename,printerror)))
		return 1;
	return SQ_ERROR; //propagates the error
}

int64_t _g_io_dofile(rabbit::VirtualMachine* v)
{
	const char *filename;
	rabbit::Bool printerror = SQFalse;
	sq_getstring(v,2,&filename);
	if(sq_gettop(v) >= 3) {
		sq_getbool(v,3,&printerror);
	}
	sq_push(v,1); //repush the this
	if(SQ_SUCCEEDED(rabbit::std::dofile(v,filename,SQTrue,printerror)))
		return 1;
	return SQ_ERROR; //propagates the error
}

#define _DECL_GLOBALIO_FUNC(name,nparams,typecheck) {#name,_g_io_##name,nparams,typecheck}
static const rabbit::RegFunction iolib_funcs[]={
	_DECL_GLOBALIO_FUNC(loadfile,-2,".sb"),
	_DECL_GLOBALIO_FUNC(dofile,-2,".sb"),
	{NULL,(SQFUNCTION)0,0,NULL}
};

rabbit::Result rabbit::std::register_iolib(rabbit::VirtualMachine* v)
{
	int64_t top = sq_gettop(v);
	//create delegate
	declare_stream(v,"file",(rabbit::UserPointer)SQSTD_FILE_TYPE_TAG, "std_file",_file_methods,iolib_funcs);
	sq_pushstring(v,"stdout",-1);
	rabbit::std::createfile(v,stdout,SQFalse);
	sq_newslot(v,-3,SQFalse);
	sq_pushstring(v,"stdin",-1);
	rabbit::std::createfile(v,stdin,SQFalse);
	sq_newslot(v,-3,SQFalse);
	sq_pushstring(v,"stderr",-1);
	rabbit::std::createfile(v,stderr,SQFalse);
	sq_newslot(v,-3,SQFalse);
	sq_settop(v,top);
	return SQ_OK;
}
