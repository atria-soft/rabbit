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
#include <rabbit/ClassMember.hpp>

namespace rabbit {
	class Class : public rabbit::RefCounted {
		public:
			Class(SQSharedState *ss,rabbit::Class *base);
		public:
			static Class* create(SQSharedState *ss, Class *base) {
				rabbit::Class *newclass = (Class *)SQ_MALLOC(sizeof(Class));
				new (newclass) Class(ss, base);
				return newclass;
			}
			~Class();
			bool newSlot(SQSharedState *ss, const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,bool bstatic);
			bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val) {
				if(_members->get(key,val)) {
					if(_isfield(val)) {
						rabbit::ObjectPtr &o = _defaultvalues[_member_idx(val)].val;
						val = _realval(o);
					} else {
						val = _methods[_member_idx(val)].val;
					}
					return true;
				}
				return false;
			}
			bool getConstructor(rabbit::ObjectPtr &ctor) {
				if(_constructoridx != -1) {
					ctor = _methods[_constructoridx].val;
					return true;
				}
				return false;
			}
			bool setAttributes(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
			bool getAttributes(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &outval);
			void lock() { _locked = true; if(_base) _base->lock(); }
			void release() {
				if (_hook) { _hook(_typetag,0);}
				sq_delete(this, Class);
			}
			void finalize();
			int64_t next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval);
			rabbit::Instance *createInstance();
			SQTable *_members;
			rabbit::Class *_base;
			etk::Vector<rabbit::ClassMember> _defaultvalues;
			etk::Vector<rabbit::ClassMember> _methods;
			rabbit::ObjectPtr _metamethods[rabbit::MT_LAST];
			rabbit::ObjectPtr _attributes;
			rabbit::UserPointer _typetag;
			SQRELEASEHOOK _hook;
			bool _locked;
			int64_t _constructoridx;
			int64_t _udsize;
	};
	#define calcinstancesize(_theclass_) \
		(_theclass_->_udsize + sq_aligning(sizeof(rabbit::Instance) +  (sizeof(rabbit::ObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0))))
}

