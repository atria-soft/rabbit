/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/sqpcheader.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/sqtable.hpp>

#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/MetaMethod.hpp>

namespace rabbit {
	class Instance : public rabbit::Delegable {
		public:
			void init(SQSharedState *ss);
			Instance(SQSharedState *ss, rabbit::Class *c, int64_t memsize);
			Instance(SQSharedState *ss, Instance *c, int64_t memsize);
		public:
			static Instance* create(SQSharedState *ss,rabbit::Class *theclass) {
				int64_t size = calcinstancesize(theclass);
				Instance *newinst = (Instance *)SQ_MALLOC(size);
				new (newinst) Instance(ss, theclass,size);
				if(theclass->_udsize) {
					newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
				}
				return newinst;
			}
			Instance *clone(SQSharedState *ss)
			{
				int64_t size = calcinstancesize(_class);
				Instance *newinst = (Instance *)SQ_MALLOC(size);
				new (newinst) Instance(ss, this,size);
				if(_class->_udsize) {
					newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
				}
				return newinst;
			}
			~Instance();
			bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val)  {
				if(_class->_members->get(key,val)) {
					if(_isfield(val)) {
						rabbit::ObjectPtr &o = _values[_member_idx(val)];
						val = _realval(o);
					}
					else {
						val = _class->_methods[_member_idx(val)].val;
					}
					return true;
				}
				return false;
			}
			bool set(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val) {
				rabbit::ObjectPtr idx;
				if(_class->_members->get(key,idx) && _isfield(idx)) {
					_values[_member_idx(idx)] = val;
					return true;
				}
				return false;
			}
			void release() {
				_uiRef++;
				if (_hook) { _hook(_userpointer,0);}
				_uiRef--;
				if(_uiRef > 0) return;
				int64_t size = _memsize;
				this->~Instance();
				SQ_FREE(this, size);
			}
			void finalize();
			bool instanceOf(rabbit::Class *trg);
			bool getMetaMethod(rabbit::VirtualMachine *v,rabbit::MetaMethod mm,rabbit::ObjectPtr &res);
			
			rabbit::Class *_class;
			rabbit::UserPointer _userpointer;
			SQRELEASEHOOK _hook;
			int64_t _memsize;
			rabbit::ObjectPtr _values[1];
	};
}

