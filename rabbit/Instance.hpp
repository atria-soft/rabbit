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
#include <rabbit/MetaMethod.hpp>
#include <rabbit/Delegable.hpp>

namespace rabbit {
	class Instance : public rabbit::Delegable {
		public:
			void init(rabbit::SharedState *ss);
			Instance(rabbit::SharedState *ss, rabbit::Class *c, int64_t memsize);
			Instance(rabbit::SharedState *ss, Instance *c, int64_t memsize);
		public:
			static Instance* create(rabbit::SharedState *ss,rabbit::Class *theclass);
			Instance *clone(rabbit::SharedState *ss);
			~Instance();
			bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val);
			bool set(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
			void release();
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

