/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/Delegable.hpp>

namespace rabbit {
	class SharedState;
	
	#define hashptr(p) ((rabbit::Hash)(((int64_t)p) >> 3))
	
	rabbit::Hash HashObj(const rabbit::ObjectPtr &key);
	
	class Table : public rabbit::Delegable {
		private:
			struct _HashNode {
				_HashNode() { next = NULL; }
				rabbit::ObjectPtr val;
				rabbit::ObjectPtr key;
				_HashNode *next;
			};
			_HashNode *_firstfree;
			_HashNode *_nodes;
			int64_t _numofnodes;
			int64_t _usednodes;
			void allocNodes(int64_t nsize);
			void Rehash(bool force);
			Table(rabbit::SharedState *ss, int64_t ninitialsize);
			void _clearNodes();
		public:
			static rabbit::Table* create(rabbit::SharedState *ss,int64_t ninitialsize);
			void finalize();
			Table *clone();
			~Table();
			_HashNode *_get(const rabbit::ObjectPtr &key,rabbit::Hash hash);
			//for compiler use
			bool getStr(const rabbit::Char* key,int64_t keylen,rabbit::ObjectPtr &val);
			bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val);
			void remove(const rabbit::ObjectPtr &key);
			bool set(const rabbit::ObjectPtr &key, const rabbit::ObjectPtr &val);
			//returns true if a new slot has been created false if it was already present
			bool newSlot(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
			int64_t next(bool getweakrefs,const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval);
			int64_t countUsed();
			void clear();
			void release();
	};
}

