/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/String.hpp>

namespace rabbit {
	
	class StringTable {
		public:
			StringTable(rabbit::SharedState*ss);
			~StringTable();
			rabbit::String *add(const rabbit::Char *,int64_t len);
			void remove(rabbit::String *);
		private:
			void resize(int64_t size);
			void allocNodes(int64_t size);
			rabbit::String **_strings;
			uint64_t _numofslots;
			uint64_t _slotused;
			rabbit::SharedState *_sharedstate;
	};
	
	#define ADD_STRING(ss,str,len) ss->_stringtable->add(str,len)
	#define REMOVE_STRING(ss,bstr) ss->_stringtable->remove(bstr)

}

