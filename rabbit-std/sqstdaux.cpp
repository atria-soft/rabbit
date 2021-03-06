/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/rabbit.hpp>
#include <rabbit-std/sqstdaux.hpp>
#include <assert.h>
#include <rabbit/StackInfos.hpp>

void rabbit::std::printcallstack(rabbit::VirtualMachine* v)
{
	SQPRINTFUNCTION pf = sq_geterrorfunc(v);
	if(pf) {
		rabbit::StackInfos si;
		int64_t i;
		float_t f;
		const char *s;
		int64_t level=1; //1 is to skip this function that is level 0
		const char *name=0;
		int64_t seq=0;
		pf(v,"\nCALLSTACK\n");
		while(SQ_SUCCEEDED(sq_stackinfos(v,level,&si)))
		{
			const char *fn="unknown";
			const char *src="unknown";
			if(si.funcname)fn=si.funcname;
			if(si.source)src=si.source;
			pf(v,"*FUNCTION [%s()] %s line [%d]\n",fn,src,si.line);
			level++;
		}
		level=0;
		pf(v,"\nLOCALS\n");

		for(level=0;level<10;level++){
			seq=0;
			while((name = sq_getlocal(v,level,seq)))
			{
				seq++;
				switch(sq_gettype(v,-1))
				{
				case rabbit::OT_NULL:
					pf(v,"[%s] NULL\n",name);
					break;
				case rabbit::OT_INTEGER:
					sq_getinteger(v,-1,&i);
					pf(v,"[%s] %d\n",name,i);
					break;
				case rabbit::OT_FLOAT:
					sq_getfloat(v,-1,&f);
					pf(v,"[%s] %.14g\n",name,f);
					break;
				case rabbit::OT_USERPOINTER:
					pf(v,"[%s] USERPOINTER\n",name);
					break;
				case rabbit::OT_STRING:
					sq_getstring(v,-1,&s);
					pf(v,"[%s] \"%s\"\n",name,s);
					break;
				case rabbit::OT_TABLE:
					pf(v,"[%s] TABLE\n",name);
					break;
				case rabbit::OT_ARRAY:
					pf(v,"[%s] ARRAY\n",name);
					break;
				case rabbit::OT_CLOSURE:
					pf(v,"[%s] CLOSURE\n",name);
					break;
				case rabbit::OT_NATIVECLOSURE:
					pf(v,"[%s] NATIVECLOSURE\n",name);
					break;
				case rabbit::OT_GENERATOR:
					pf(v,"[%s] GENERATOR\n",name);
					break;
				case rabbit::OT_USERDATA:
					pf(v,"[%s] USERDATA\n",name);
					break;
				case rabbit::OT_THREAD:
					pf(v,"[%s] THREAD\n",name);
					break;
				case rabbit::OT_CLASS:
					pf(v,"[%s] CLASS\n",name);
					break;
				case rabbit::OT_INSTANCE:
					pf(v,"[%s] INSTANCE\n",name);
					break;
				case rabbit::OT_WEAKREF:
					pf(v,"[%s] WEAKREF\n",name);
					break;
				case rabbit::OT_BOOL:
					{
						rabbit::Bool bval;
						sq_getbool(v,-1,&bval);
						pf(v,"[%s] %s\n",name,bval == SQTrue ? "true":"false");
					}
					break;
				default: assert(0); break;
				}
				sq_pop(v,1);
			}
		}
	}
}

namespace rabbit {
	namespace std {
		static int64_t aux_printerror(rabbit::VirtualMachine* v) {
			SQPRINTFUNCTION pf = sq_geterrorfunc(v);
			if(pf) {
				const char *sErr = 0;
				if(sq_gettop(v)>=1) {
					if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))   {
						pf(v,"\nAN ERROR HAS OCCURRED [%s]\n",sErr);
					}
					else{
						pf(v,"\nAN ERROR HAS OCCURRED [unknown]\n");
					}
					rabbit::std::printcallstack(v);
				}
			}
			return 0;
		}
		
		void compiler_error(rabbit::VirtualMachine* v,const char *sErr,const char *sSource,int64_t line,int64_t column) {
			SQPRINTFUNCTION pf = sq_geterrorfunc(v);
			if(pf) {
				pf(v,"%s line = (%d) column = (%d) : error %s\n",sSource,line,column,sErr);
			}
		}
	}
}


void rabbit::std::seterrorhandlers(rabbit::VirtualMachine* v)
{
	sq_setcompilererrorhandler(v,rabbit::std::compiler_error);
	sq_newclosure(v,rabbit::std::aux_printerror,0);
	sq_seterrorhandler(v);
}
