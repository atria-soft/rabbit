/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>

namespace rabbit {
	#define _CALC_NATVIVECLOSURE_SIZE(noutervalues) (sizeof(rabbit::NativeClosure) + (noutervalues*sizeof(rabbit::ObjectPtr)))
	class NativeClosure : public rabbit::RefCounted {
		private:
			rabbit::NativeClosure(rabbit::SharedState *ss,SQFUNCTION func){
				_function=func;
				_env = NULL;
			}
		public:
			static rabbit::NativeClosure *create(rabbit::SharedState *ss,SQFUNCTION func,int64_t nouters)
			{
				int64_t size = _CALC_NATVIVECLOSURE_SIZE(nouters);
				rabbit::NativeClosure *nc=(rabbit::NativeClosure*)SQ_MALLOC(size);
				new (nc) rabbit::NativeClosure(ss,func);
				nc->_outervalues = (rabbit::ObjectPtr *)(nc + 1);
				nc->_noutervalues = nouters;
				_CONSTRUCT_VECTOR(rabbit::ObjectPtr,nc->_noutervalues,nc->_outervalues);
				return nc;
			}
			rabbit::NativeClosure *clone()
			{
				rabbit::NativeClosure * ret = rabbit::NativeClosure::create(NULL,_function,_noutervalues);
				ret->_env = _env;
				if(ret->_env) __ObjaddRef(ret->_env);
				ret->_name = _name;
				_COPY_VECTOR(ret->_outervalues,_outervalues,_noutervalues);
				ret->_typecheck = _typecheck;
				ret->_nparamscheck = _nparamscheck;
				return ret;
			}
			~rabbit::NativeClosure()
			{
				__Objrelease(_env);
			}
			void release(){
				int64_t size = _CALC_NATVIVECLOSURE_SIZE(_noutervalues);
				_DESTRUCT_VECTOR(ObjectPtr,_noutervalues,_outervalues);
				this->~rabbit::NativeClosure();
				sq_free(this,size);
			}
		
			int64_t _nparamscheck;
			etk::Vector<int64_t> _typecheck;
			rabbit::ObjectPtr *_outervalues;
			uint64_t _noutervalues;
			rabbit::WeakRef *_env;
			SQFUNCTION _function;
			rabbit::ObjectPtr _name;
	};

}

