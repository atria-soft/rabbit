/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	
	#define _CALC_CLOSURE_SIZE(func) (sizeof(rabbit::Closure) + (func->_noutervalues*sizeof(rabbit::ObjectPtr)) + (func->_ndefaultparams*sizeof(rabbit::ObjectPtr)))
	
	class Closure : public rabbit::RefCounted {
		private:
			Closure(rabbit::SharedState *ss,rabbit::FunctionProto *func){
				_function = func;
				__ObjaddRef(_function); _base = NULL;
				_env = NULL;
				_root=NULL;
			}
		public:
			static rabbit::Closure *create(rabbit::SharedState *ss,rabbit::FunctionProto *func,rabbit::WeakRef *root){
				int64_t size = _CALC_CLOSURE_SIZE(func);
				rabbit::Closure *nc=(rabbit::Closure*)SQ_MALLOC(size);
				new (nc) rabbit::Closure(ss,func);
				nc->_outervalues = (rabbit::ObjectPtr *)(nc + 1);
				nc->_defaultparams = &nc->_outervalues[func->_noutervalues];
				nc->_root = root;
				 __ObjaddRef(nc->_root);
				_CONSTRUCT_VECTOR(rabbit::ObjectPtr,func->_noutervalues,nc->_outervalues);
				_CONSTRUCT_VECTOR(rabbit::ObjectPtr,func->_ndefaultparams,nc->_defaultparams);
				return nc;
			}
			void release(){
				rabbit::FunctionProto *f = _function;
				int64_t size = _CALC_CLOSURE_SIZE(f);
				_DESTRUCT_VECTOR(ObjectPtr,f->_noutervalues,_outervalues);
				_DESTRUCT_VECTOR(ObjectPtr,f->_ndefaultparams,_defaultparams);
				__Objrelease(_function);
				this->~rabbit::Closure();
				sq_vm_free(this,size);
			}
			void setRoot(rabbit::WeakRef *r)
			{
				__Objrelease(_root);
				_root = r;
				__ObjaddRef(_root);
			}
			rabbit::Closure *clone()
			{
				rabbit::FunctionProto *f = _function;
				rabbit::Closure * ret = rabbit::Closure::create(NULL,f,_root);
				ret->_env = _env;
				if(ret->_env) __ObjaddRef(ret->_env);
				_COPY_VECTOR(ret->_outervalues,_outervalues,f->_noutervalues);
				_COPY_VECTOR(ret->_defaultparams,_defaultparams,f->_ndefaultparams);
				return ret;
			}
			~rabbit::Closure();
		
			bool save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write);
			static bool load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret);
			rabbit::WeakRef *_env;
			rabbit::WeakRef *_root;
			rabbit::Class *_base;
			rabbit::FunctionProto *_function;
			rabbit::ObjectPtr *_outervalues;
			rabbit::ObjectPtr *_defaultparams;
	};

}

#define SQ_CLOSURESTREAM_HEAD (('S'<<24)|('Q'<<16)|('I'<<8)|('R'))
#define SQ_CLOSURESTREAM_PART (('P'<<24)|('A'<<16)|('R'<<8)|('T'))
#define SQ_CLOSURESTREAM_TAIL (('T'<<24)|('A'<<16)|('I'<<8)|('L'))
