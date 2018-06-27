/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_ltable.c.html
*/

#include <rabbit/sqstring.hpp>
#include <rabbit/Delegable.hpp>
#include <rabbit/squtils.hpp>
#include <etk/Allocator.hpp>
#include <rabbit/Object.hpp>
#include <rabbit/WeakRef.hpp>


#define hashptr(p)  ((SQHash)(((int64_t)p) >> 3))

inline SQHash HashObj(const rabbit::ObjectPtr &key)
{
	switch(sq_type(key)) {
		case rabbit::OT_STRING:
			return _string(key)->_hash;
		case rabbit::OT_FLOAT:
			return (SQHash)((int64_t)_float(key));
		case rabbit::OT_BOOL:
		case rabbit::OT_INTEGER:
			return (SQHash)((int64_t)_integer(key));
		default:
			return hashptr(key._unVal.pRefCounted);
	}
}

struct SQTable : public rabbit::Delegable
{
private:
	struct _HashNode
	{
		_HashNode() { next = NULL; }
		rabbit::ObjectPtr val;
		rabbit::ObjectPtr key;
		_HashNode *next;
	};
	_HashNode *_firstfree;
	_HashNode *_nodes;
	int64_t _numofnodes;
	int64_t _usednodes;

///////////////////////////
	void allocNodes(int64_t nsize);
	void Rehash(bool force);
	SQTable(SQSharedState *ss, int64_t ninitialsize);
	void _clearNodes();
public:
	static SQTable* create(SQSharedState *ss,int64_t ninitialsize)
	{
		char* tmp = (char*)SQ_MALLOC(sizeof(SQTable));
		SQTable *newtable = new (tmp) SQTable(ss, ninitialsize);
		newtable->_delegate = NULL;
		return newtable;
	}
	void finalize();
	SQTable *clone();
	~SQTable()
	{
		setDelegate(NULL);
		for (int64_t i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
		SQ_FREE(_nodes, _numofnodes * sizeof(_HashNode));
	}
	inline _HashNode *_get(const rabbit::ObjectPtr &key,SQHash hash)
	{
		_HashNode *n = &_nodes[hash];
		do{
			if(_rawval(n->key) == _rawval(key) && sq_type(n->key) == sq_type(key)){
				return n;
			}
		}while((n = n->next));
		return NULL;
	}
	//for compiler use
	inline bool getStr(const rabbit::Char* key,int64_t keylen,rabbit::ObjectPtr &val)
	{
		SQHash hash = _hashstr(key,keylen);
		_HashNode *n = &_nodes[hash & (_numofnodes - 1)];
		_HashNode *res = NULL;
		do{
			if(sq_type(n->key) == rabbit::OT_STRING && (scstrcmp(_stringval(n->key),key) == 0)){
				res = n;
				break;
			}
		}while((n = n->next));
		if (res) {
			val = _realval(res->val);
			return true;
		}
		return false;
	}
	bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val);
	void remove(const rabbit::ObjectPtr &key);
	bool set(const rabbit::ObjectPtr &key, const rabbit::ObjectPtr &val);
	//returns true if a new slot has been created false if it was already present
	bool newSlot(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
	int64_t next(bool getweakrefs,const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval);

	int64_t countUsed(){ return _usednodes;}
	void clear();
	void release()
	{
		sq_delete(this, SQTable);
	}

};

