/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/MetaMethod.hpp>
#include <rabbit/ObjectPtr.hpp>

namespace rabbit {
	class Table;
	class VirtualMachine;
	class Delegable : public rabbit::RefCounted {
		public:
			bool setDelegate(rabbit::Table *m);
		public:
			virtual bool getMetaMethod(rabbit::VirtualMachine *v, rabbit::MetaMethod mm, rabbit::ObjectPtr& res);
			rabbit::Table *_delegate;
	};
}
