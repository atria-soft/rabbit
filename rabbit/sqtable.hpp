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


#define hashptr(p)  ((SQHash)(((int64_t)p) >> 3))

inline SQHash HashObj(const SQObjectPtr &key)
{
	switch(sq_type(key)) {
		case OT_STRING:	 return _string(key)->_hash;
		case OT_FLOAT:	  return (SQHash)((int64_t)_float(key));
		case OT_BOOL: case OT_INTEGER:  return (SQHash)((int64_t)_integer(key));
		default:			return hashptr(key._unVal.pRefCounted);
	}
}

struct SQTable : public SQDelegable
{
private:
	struct _HashNode
	{
		_HashNode() { next = NULL; }
		SQObjectPtr val;
		SQObjectPtr key;
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
		SQTable *newtable = (SQTable*)SQ_MALLOC(sizeof(SQTable));
		new (newtable) SQTable(ss, ninitialsize);
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
	inline _HashNode *_get(const SQObjectPtr &key,SQHash hash)
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
	inline bool getStr(const SQChar* key,int64_t keylen,SQObjectPtr &val)
	{
		SQHash hash = _hashstr(key,keylen);
		_HashNode *n = &_nodes[hash & (_numofnodes - 1)];
		_HashNode *res = NULL;
		do{
			if(sq_type(n->key) == OT_STRING && (scstrcmp(_stringval(n->key),key) == 0)){
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
	bool get(const SQObjectPtr &key,SQObjectPtr &val);
	void remove(const SQObjectPtr &key);
	bool set(const SQObjectPtr &key, const SQObjectPtr &val);
	//returns true if a new slot has been created false if it was already present
	bool newSlot(const SQObjectPtr &key,const SQObjectPtr &val);
	int64_t next(bool getweakrefs,const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);

	int64_t countUsed(){ return _usednodes;}
	void clear();
	void release()
	{
		sq_delete(this, SQTable);
	}

};

