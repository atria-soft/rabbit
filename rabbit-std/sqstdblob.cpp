/* see copyright notice in rabbit.hpp */
#include <new>
#include <rabbit/rabbit.hpp>
#include <rabbit-std/sqstdio.hpp>
#include <string.h>
#include <rabbit-std/sqstdblob.hpp>
#include <rabbit-std/sqstdstream.hpp>
#include <rabbit-std/sqstdblobimpl.hpp>

#define SQSTD_BLOB_TYPE_TAG ((uint64_t)(SQSTD_STREAM_TYPE_TAG | 0x00000002))

//Blob


#define SETUP_BLOB(v) \
	rabbit::std::Blob *self = NULL; \
	{ if(SQ_FAILED(rabbit::sq_getinstanceup(v,1,(rabbit::UserPointer*)&self,(rabbit::UserPointer)SQSTD_BLOB_TYPE_TAG))) \
		return rabbit::sq_throwerror(v,"invalid type tag");  } \
	if(!self || !self->IsValid())  \
		return rabbit::sq_throwerror(v,"the blob is invalid");


static int64_t _blob_resize(rabbit::VirtualMachine* v)
{
	SETUP_BLOB(v);
	int64_t size;
	rabbit::sq_getinteger(v,2,&size);
	if(!self->resize(size))
		return rabbit::sq_throwerror(v,"resize failed");
	return 0;
}

static void __swap_dword(unsigned int *n)
{
	*n=(unsigned int)(((*n&0xFF000000)>>24)  |
			((*n&0x00FF0000)>>8)  |
			((*n&0x0000FF00)<<8)  |
			((*n&0x000000FF)<<24));
}

static void __swap_word(unsigned short *n)
{
	*n=(unsigned short)((*n>>8)&0x00FF)| ((*n<<8)&0xFF00);
}

static int64_t _blob_swap4(rabbit::VirtualMachine* v)
{
	SETUP_BLOB(v);
	int64_t num=(self->Len()-(self->Len()%4))>>2;
	unsigned int *t=(unsigned int *)self->getBuf();
	for(int64_t i = 0; i < num; i++) {
		__swap_dword(&t[i]);
	}
	return 0;
}

static int64_t _blob_swap2(rabbit::VirtualMachine* v)
{
	SETUP_BLOB(v);
	int64_t num=(self->Len()-(self->Len()%2))>>1;
	unsigned short *t = (unsigned short *)self->getBuf();
	for(int64_t i = 0; i < num; i++) {
		__swap_word(&t[i]);
	}
	return 0;
}

static int64_t _blob__set(rabbit::VirtualMachine* v)
{
	SETUP_BLOB(v);
	int64_t idx,val;
	rabbit::sq_getinteger(v,2,&idx);
	rabbit::sq_getinteger(v,3,&val);
	if(idx < 0 || idx >= self->Len())
		return rabbit::sq_throwerror(v,"index out of range");
	((unsigned char *)self->getBuf())[idx] = (unsigned char) val;
	rabbit::sq_push(v,3);
	return 1;
}

static int64_t _blob__get(rabbit::VirtualMachine* v)
{
	SETUP_BLOB(v);
	int64_t idx;
	
	if ((rabbit::sq_gettype(v, 2) & SQOBJECT_NUMERIC) == 0)
	{
		rabbit::sq_pushnull(v);
		return rabbit::sq_throwobject(v);
	}
	rabbit::sq_getinteger(v,2,&idx);
	if(idx < 0 || idx >= self->Len())
		return rabbit::sq_throwerror(v,"index out of range");
	rabbit::sq_pushinteger(v,((unsigned char *)self->getBuf())[idx]);
	return 1;
}

static int64_t _blob__nexti(rabbit::VirtualMachine* v)
{
	SETUP_BLOB(v);
	if(rabbit::sq_gettype(v,2) == rabbit::OT_NULL) {
		rabbit::sq_pushinteger(v, 0);
		return 1;
	}
	int64_t idx;
	if(SQ_SUCCEEDED(rabbit::sq_getinteger(v, 2, &idx))) {
		if(idx+1 < self->Len()) {
			rabbit::sq_pushinteger(v, idx+1);
			return 1;
		}
		rabbit::sq_pushnull(v);
		return 1;
	}
	return rabbit::sq_throwerror(v,"internal error (_nexti) wrong argument type");
}

static int64_t _blob__typeof(rabbit::VirtualMachine* v)
{
	rabbit::sq_pushstring(v,"blob",-1);
	return 1;
}

static int64_t _blob_releasehook(rabbit::UserPointer p, int64_t SQ_UNUSED_ARG(size))
{
	rabbit::std::Blob *self = (rabbit::std::Blob*)p;
	self->~Blob();
	rabbit::sq_free(self,sizeof(rabbit::std::Blob));
	return 1;
}

static int64_t _blob_constructor(rabbit::VirtualMachine* v)
{
	int64_t nparam = rabbit::sq_gettop(v);
	int64_t size = 0;
	if(nparam == 2) {
		rabbit::sq_getinteger(v, 2, &size);
	}
	if(size < 0) return rabbit::sq_throwerror(v, "cannot create blob with negative size");
	//rabbit::std::Blob *b = new rabbit::std::Blob(size);

	rabbit::std::Blob *b = new (rabbit::sq_malloc(sizeof(rabbit::std::Blob)))rabbit::std::Blob(size);
	if(SQ_FAILED(rabbit::sq_setinstanceup(v,1,b))) {
		b->~Blob();
		rabbit::sq_free(b,sizeof(rabbit::std::Blob));
		return rabbit::sq_throwerror(v, "cannot create blob");
	}
	rabbit::sq_setreleasehook(v,1,_blob_releasehook);
	return 0;
}

static int64_t _blob__cloned(rabbit::VirtualMachine* v)
{
	rabbit::std::Blob *other = NULL;
	{
		if(SQ_FAILED(rabbit::sq_getinstanceup(v,2,(rabbit::UserPointer*)&other,(rabbit::UserPointer)SQSTD_BLOB_TYPE_TAG)))
			return SQ_ERROR;
	}
	//rabbit::std::Blob *thisone = new rabbit::std::Blob(other->Len());
	rabbit::std::Blob *thisone = new (rabbit::sq_malloc(sizeof(rabbit::std::Blob)))rabbit::std::Blob(other->Len());
	memcpy(thisone->getBuf(),other->getBuf(),thisone->Len());
	if(SQ_FAILED(rabbit::sq_setinstanceup(v,1,thisone))) {
		thisone->~Blob();
		rabbit::sq_free(thisone,sizeof(rabbit::std::Blob));
		return rabbit::sq_throwerror(v, "cannot clone blob");
	}
	rabbit::sq_setreleasehook(v,1,_blob_releasehook);
	return 0;
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {#name,_blob_##name,nparams,typecheck}
static const rabbit::RegFunction _blob_methods[] = {
	_DECL_BLOB_FUNC(constructor,-1,"xn"),
	_DECL_BLOB_FUNC(resize,2,"xn"),
	_DECL_BLOB_FUNC(swap2,1,"x"),
	_DECL_BLOB_FUNC(swap4,1,"x"),
	_DECL_BLOB_FUNC(_set,3,"xnn"),
	_DECL_BLOB_FUNC(_get,2,"x."),
	_DECL_BLOB_FUNC(_typeof,1,"x"),
	_DECL_BLOB_FUNC(_nexti,2,"x"),
	_DECL_BLOB_FUNC(_cloned,2,"xx"),
	{NULL,(SQFUNCTION)0,0,NULL}
};



//GLOBAL FUNCTIONS

static int64_t _g_blob_casti2f(rabbit::VirtualMachine* v)
{
	int64_t i;
	rabbit::sq_getinteger(v,2,&i);
	rabbit::sq_pushfloat(v,*((const float_t *)&i));
	return 1;
}

static int64_t _g_blob_castf2i(rabbit::VirtualMachine* v)
{
	float_t f;
	rabbit::sq_getfloat(v,2,&f);
	rabbit::sq_pushinteger(v,*((const int64_t *)&f));
	return 1;
}

static int64_t _g_blob_swap2(rabbit::VirtualMachine* v)
{
	int64_t i;
	rabbit::sq_getinteger(v,2,&i);
	short s=(short)i;
	rabbit::sq_pushinteger(v,(s<<8)|((s>>8)&0x00FF));
	return 1;
}

static int64_t _g_blob_swap4(rabbit::VirtualMachine* v)
{
	int64_t i;
	rabbit::sq_getinteger(v,2,&i);
	unsigned int t4 = (unsigned int)i;
	__swap_dword(&t4);
	rabbit::sq_pushinteger(v,(int64_t)t4);
	return 1;
}

static int64_t _g_blob_swapfloat(rabbit::VirtualMachine* v)
{
	float_t f;
	rabbit::sq_getfloat(v,2,&f);
	__swap_dword((unsigned int *)&f);
	rabbit::sq_pushfloat(v,f);
	return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {#name,_g_blob_##name,nparams,typecheck}
static const rabbit::RegFunction bloblib_funcs[]={
	_DECL_GLOBALBLOB_FUNC(casti2f,2,".n"),
	_DECL_GLOBALBLOB_FUNC(castf2i,2,".n"),
	_DECL_GLOBALBLOB_FUNC(swap2,2,".n"),
	_DECL_GLOBALBLOB_FUNC(swap4,2,".n"),
	_DECL_GLOBALBLOB_FUNC(swapfloat,2,".n"),
	{NULL,(SQFUNCTION)0,0,NULL}
};

rabbit::Result rabbit::std::getblob(rabbit::VirtualMachine* v,int64_t idx,rabbit::UserPointer *ptr)
{
	rabbit::std::Blob *blob;
	if(SQ_FAILED(rabbit::sq_getinstanceup(v,idx,(rabbit::UserPointer *)&blob,(rabbit::UserPointer)SQSTD_BLOB_TYPE_TAG)))
		return -1;
	*ptr = blob->getBuf();
	return SQ_OK;
}

int64_t rabbit::std::getblobsize(rabbit::VirtualMachine* v,int64_t idx)
{
	rabbit::std::Blob *blob;
	if(SQ_FAILED(rabbit::sq_getinstanceup(v,idx,(rabbit::UserPointer *)&blob,(rabbit::UserPointer)SQSTD_BLOB_TYPE_TAG)))
		return -1;
	return blob->Len();
}

rabbit::UserPointer rabbit::std::createblob(rabbit::VirtualMachine* v, int64_t size)
{
	int64_t top = rabbit::sq_gettop(v);
	rabbit::sq_pushregistrytable(v);
	rabbit::sq_pushstring(v,"std_blob",-1);
	if(SQ_SUCCEEDED(rabbit::sq_get(v,-2))) {
		rabbit::sq_remove(v,-2); //removes the registry
		rabbit::sq_push(v,1); // push the this
		rabbit::sq_pushinteger(v,size); //size
		rabbit::std::Blob *blob = NULL;
		if(SQ_SUCCEEDED(rabbit::sq_call(v,2,SQTrue,SQFalse))
			&& SQ_SUCCEEDED(rabbit::sq_getinstanceup(v,-1,(rabbit::UserPointer *)&blob,(rabbit::UserPointer)SQSTD_BLOB_TYPE_TAG))) {
			rabbit::sq_remove(v,-2);
			return blob->getBuf();
		}
	}
	rabbit::sq_settop(v,top);
	return NULL;
}

rabbit::Result rabbit::std::register_bloblib(rabbit::VirtualMachine* v)
{
	return declare_stream(v,"blob",(rabbit::UserPointer)SQSTD_BLOB_TYPE_TAG,"std_blob",_blob_methods,bloblib_funcs);
}

