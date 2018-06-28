/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/ObjectPtr.hpp>
#include <rabbit/Object.hpp>
#include <rabbit/Hash.hpp>

namespace rabbit {
	class RefTable {
		public:
			class RefNode {
				public:
					rabbit::ObjectPtr obj;
					uint64_t refs;
					struct RefNode *next;
			};
			RefTable();
			~RefTable();
			void addRef(rabbit::Object &obj);
			rabbit::Bool release(rabbit::Object &obj);
			uint64_t getRefCount(rabbit::Object &obj);
			void finalize();
		private:
			RefNode *get(rabbit::Object &obj,rabbit::Hash &mainpos,RefNode **prev,bool add);
			RefNode *add(rabbit::Hash mainpos,rabbit::Object &obj);
			void resize(uint64_t size);
			void allocNodes(uint64_t size);
			uint64_t _numofslots;
			uint64_t _slotused;
			RefNode *_nodes;
			RefNode *_freelist;
			RefNode **_buckets;
	};
}

