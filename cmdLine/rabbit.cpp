/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <rabbit/rabbit.hpp>
#include <rabbit-std/sqstdblob.hpp>
#include <rabbit-std/sqstdsystem.hpp>
#include <rabbit-std/sqstdio.hpp>
#include <rabbit-std/sqstdmath.hpp>
#include <rabbit-std/sqstdstring.hpp>
#include <rabbit-std/sqstdaux.hpp>

void PrintVersionInfos();

#if defined(_MSC_VER) && defined(_DEBUG)
int MemAllocHook(int allocType,
				 void *userData,
				 size_t size,
				 int blockType,
				 long requestNumber,
				 const unsigned char *filename,
				 int lineNumber) {
	//if(requestNumber==769)_asm int 3;
	return 1;
}
#endif


int64_t quit(rabbit::VirtualMachine* v)
{
	int *done;
	sq_getuserpointer(v,-1,(rabbit::UserPointer*)&done);
	*done=1;
	return 0;
}

void printfunc(rabbit::VirtualMachine* SQ_UNUSED_ARG(v),const char *s,...)
{
	va_list vl;
	va_start(vl, s);
	vfprintf(stdout, s, vl);
	va_end(vl);
}

void errorfunc(rabbit::VirtualMachine* SQ_UNUSED_ARG(v),const char *s,...)
{
	va_list vl;
	va_start(vl, s);
	vfprintf(stderr, s, vl);
	va_end(vl);
}

void PrintVersionInfos()
{
	fprintf(stdout,"%s %s (%d bits)\n",RABBIT_VERSION,RABBIT_COPYRIGHT,((int)(sizeof(int64_t)*8)));
}

void PrintUsage()
{
	fprintf(stderr,"usage: sq <options> <scriptpath [args]>.\n"
		"Available options are:\n"
		"   -c			  compiles the file to bytecode(default output 'out.karrot')\n"
		"   -o			  specifies output file for the -c option\n"
		"   -c			  compiles only\n"
		"   -d			  generates debug infos\n"
		"   -v			  displays version infos\n"
		"   -h			  prints help\n");
}

#define _INTERACTIVE 0
#define _DONE 2
#define _ERROR 3
//<<FIXME>> this func is a mess
int getargs(rabbit::VirtualMachine* v,int argc, char* argv[],int64_t *retval)
{
	int i;
	int compiles_only = 0;
	char * output = NULL;
	*retval = 0;
	if(argc>1)
	{
		int arg=1,exitloop=0;

		while(arg < argc && !exitloop)
		{

			if(argv[arg][0]=='-')
			{
				switch(argv[arg][1])
				{
				case 'd':
					sq_enabledebuginfo(v,1);
					break;
				case 'c':
					compiles_only = 1;
					break;
				case 'o':
					if(arg < argc) {
						arg++;
						output = argv[arg];
					}
					break;
				case 'v':
					PrintVersionInfos();
					return _DONE;

				case 'h':
					PrintVersionInfos();
					PrintUsage();
					return _DONE;
				default:
					PrintVersionInfos();
					printf("unknown prameter '-%c'\n",argv[arg][1]);
					PrintUsage();
					*retval = -1;
					return _ERROR;
				}
			}else break;
			arg++;
		}

		// src file

		if(arg<argc) {
			const char *filename=NULL;
			filename=argv[arg];

			arg++;

			//sq_pushstring(v,"ARGS",-1);
			//sq_newarray(v,0);

			//sq_createslot(v,-3);
			//sq_pop(v,1);
			if(compiles_only) {
				if(SQ_SUCCEEDED(rabbit::std::loadfile(v,filename,SQTrue))){
					const char *outfile = "out.karrot";
					if(output) {
						outfile = output;
					}
					if(SQ_SUCCEEDED(rabbit::std::writeclosuretofile(v,outfile)))
						return _DONE;
				}
			}
			else {
				//if(SQ_SUCCEEDED(rabbit::std::dofile(v,filename,SQFalse,SQTrue))) {
					//return _DONE;
				//}
				if(SQ_SUCCEEDED(rabbit::std::loadfile(v,filename,SQTrue))) {
					int callargs = 1;
					sq_pushroottable(v);
					for(i=arg;i<argc;i++)
					{
						const char *a;
						a=argv[i];
						sq_pushstring(v,a,-1);
						callargs++;
						//sq_arrayappend(v,-2);
					}
					if(SQ_SUCCEEDED(sq_call(v,callargs,SQTrue,SQTrue))) {
						rabbit::ObjectType type = sq_gettype(v,-1);
						if(type == rabbit::OT_INTEGER) {
							*retval = type;
							sq_getinteger(v,-1,retval);
						}
						return _DONE;
					}
					else{
						return _ERROR;
					}

				}
			}
			//if this point is reached an error occurred
			{
				const char *err;
				sq_getlasterror(v);
				if(SQ_SUCCEEDED(sq_getstring(v,-1,&err))) {
					printf("error [%s]\n",err);
					*retval = -2;
					return _ERROR;
				}
			}

		}
	}

	return _INTERACTIVE;
}

void Interactive(rabbit::VirtualMachine* v)
{

#define MAXINPUT 1024
	char buffer[MAXINPUT];
	int64_t blocks =0;
	int64_t string=0;
	int64_t retval=0;
	int64_t done=0;
	PrintVersionInfos();

	sq_pushroottable(v);
	sq_pushstring(v,"quit",-1);
	sq_pushuserpointer(v,&done);
	sq_newclosure(v,quit,1);
	sq_setparamscheck(v,1,NULL);
	sq_newslot(v,-3,SQFalse);
	sq_pop(v,1);

	while (!done)
	{
		int64_t i = 0;
		printf("\nrabbit> ");
		for(;;) {
			int c;
			if(done)return;
			c = getchar();
			if (c == '\n') {
				if (i>0 && buffer[i-1] == '\\')
				{
					buffer[i-1] = '\n';
				}
				else if(blocks==0)break;
				buffer[i++] = '\n';
			}
			else if (c=='}') {blocks--; buffer[i++] = (char)c;}
			else if(c=='{' && !string){
					blocks++;
					buffer[i++] = (char)c;
			}
			else if(c=='"' || c=='\''){
					string=!string;
					buffer[i++] = (char)c;
			}
			else if (i >= MAXINPUT-1) {
				fprintf(stderr, "rabbit: input line too long\n");
				break;
			}
			else{
				buffer[i++] = (char)c;
			}
		}
		buffer[i] = '\0';

		if(buffer[0]=='='){
			snprintf(sq_getscratchpad(v,MAXINPUT),(size_t)MAXINPUT,"return (%s)",&buffer[1]);
			memcpy(buffer,sq_getscratchpad(v,-1),(strlen(sq_getscratchpad(v,-1))+1)*sizeof(char));
			retval=1;
		}
		i=strlen(buffer);
		if(i>0){
			int64_t oldtop=sq_gettop(v);
			if(SQ_SUCCEEDED(sq_compilebuffer(v,buffer,i,"interactive console",SQTrue))){
				sq_pushroottable(v);
				if(SQ_SUCCEEDED(sq_call(v,1,retval,SQTrue)) &&  retval){
					printf("\n");
					sq_pushroottable(v);
					sq_pushstring(v,"print",-1);
					sq_get(v,-2);
					sq_pushroottable(v);
					sq_push(v,-4);
					sq_call(v,2,SQFalse,SQTrue);
					retval=0;
					printf("\n");
				}
			}

			sq_settop(v,oldtop);
		}
	}
}

int main(int argc, char* argv[])
{
	rabbit::VirtualMachine* v;
	int64_t retval = 0;
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtsetAllocHook(MemAllocHook);
#endif

	v=rabbit::sq_open(1024);
	sq_setprintfunc(v,printfunc,errorfunc);

	sq_pushroottable(v);

	rabbit::std::register_bloblib(v);
	rabbit::std::register_iolib(v);
	rabbit::std::register_systemlib(v);
	rabbit::std::register_mathlib(v);
	rabbit::std::register_stringlib(v);

	//aux library
	//sets error handlers
	rabbit::std::seterrorhandlers(v);

	//gets arguments
	switch(getargs(v,argc,argv,&retval))
	{
	case _INTERACTIVE:
		Interactive(v);
		break;
	case _DONE:
	case _ERROR:
	default:
		break;
	}

	sq_close(v);

#if defined(_MSC_VER) && defined(_DEBUG)
	_getch();
	_CrtMemdumpAllObjectsSince( NULL );
#endif
	return retval;
}

