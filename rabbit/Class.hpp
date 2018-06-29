/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>

#include <rabbit/VirtualMachine.hpp>

#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/ClassMember.hpp>

namespace rabbit {
	class Class : public rabbit::RefCounted {
		public:
			Class(rabbit::SharedState *ss,rabbit::Class *base);
		public:
			static Class* create(rabbit::SharedState *ss, Class *base);
			~Class();
			bool newSlot(rabbit::SharedState *ss, const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,bool bstatic);
			bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val);
			bool getConstructor(rabbit::ObjectPtr &ctor);
			bool setAttributes(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
			bool getAttributes(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &outval);
			void lock();
			void release();
			void finalize();
			int64_t next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval);
			rabbit::Instance *createInstance();
			rabbit::Table *_members;
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

};

