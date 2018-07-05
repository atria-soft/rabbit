/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Table.hpp>
#include <rabbit/String.hpp>
#include <rabbit/WeakRef.hpp>
#include <etk/Allocator.hpp>

rabbit::Hash rabbit::HashObj(const rabbit::ObjectPtr &key) {
	switch(key.getType()) {
		case rabbit::OT_STRING:
			return key.toString()->_hash;
		case rabbit::OT_FLOAT:
			return (rabbit::Hash)((int64_t)key.toFloat());
		case rabbit::OT_BOOL:
		case rabbit::OT_INTEGER:
			return (rabbit::Hash)((int64_t)key.toInteger());
		default:
			return hashptr(key._unVal.pRefCounted);
	}
}


#define MINPOWER2 4

rabbit::Table::Table(rabbit::SharedState *ss,int64_t ninitialsize) {
	int64_t pow2size=MINPOWER2;
	while(ninitialsize>pow2size)pow2size=pow2size<<1;
	allocNodes(pow2size);
	_usednodes = 0;
	_delegate = NULL;
}

void rabbit::Table::remove(const rabbit::ObjectPtr &key) const {
	_HashNode *n = _get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		n->val.Null();
		n->key.Null();
		_usednodes--;
		Rehash(false);
	}
}

void rabbit::Table::allocNodes(int64_t nsize) const
{
	_HashNode *nodes=(_HashNode *)SQ_MALLOC(sizeof(_HashNode)*nsize);
	for(int64_t i=0;i<nsize;i++){
		_HashNode &n = nodes[i];
		new ((char*)&n) _HashNode;
		n.next=NULL;
	}
	_numofnodes=nsize;
	_nodes=nodes;
	_firstfree=&_nodes[_numofnodes-1];
}

void rabbit::Table::Rehash(bool force) const
{
	int64_t oldsize=_numofnodes;
	//prevent problems with the integer division
	if(oldsize<4) {
		oldsize=4;
	}
	_HashNode *nold=_nodes;
	int64_t nelems=countUsed();
	if (nelems >= oldsize-oldsize/4) {
		/* using more than 3/4? */
		allocNodes(oldsize*2);
	} else if (    nelems <= oldsize/4 /* less than 1/4? */
	            && oldsize > MINPOWER2) {
		allocNodes(oldsize/2);
	} else if(force) {
		allocNodes(oldsize);
	} else {
		return;
	}
	_usednodes = 0;
	for (int64_t i=0; i<oldsize; i++) {
		_HashNode *old = nold+i;
		if (old->key.isNull() == false) {
			newSlot(old->key,old->val);
		}
	}
	for(int64_t k=0;k<oldsize;k++)
		nold[k].~_HashNode();
	SQ_FREE(nold,oldsize*sizeof(_HashNode));
}

rabbit::Table *rabbit::Table::clone() const
{
	rabbit::Table *nt=create(NULL,_numofnodes);
#ifdef _FAST_CLONE
	_HashNode *basesrc = _nodes;
	_HashNode *basedst = nt->_nodes;
	_HashNode *src = _nodes;
	_HashNode *dst = nt->_nodes;
	int64_t n = 0;
	for(n = 0; n < _numofnodes; n++) {
		dst->key = src->key;
		dst->val = src->val;
		if(src->next) {
			assert(src->next > basesrc);
			dst->next = basedst + (src->next - basesrc);
			assert(dst != dst->next);
		}
		dst++;
		src++;
	}
	assert(_firstfree > basesrc);
	assert(_firstfree != NULL);
	nt->_firstfree = basedst + (_firstfree - basesrc);
	nt->_usednodes = _usednodes;
#else
	int64_t ridx=0;
	rabbit::ObjectPtr key,val;
	while((ridx=next(true,ridx,key,val))!=-1){
		nt->newSlot(key,val);
	}
#endif
	nt->setDelegate(_delegate);
	return nt;
}

bool rabbit::Table::get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val) const
{
	if(key.isNull() == true)
		return false;
	_HashNode *n = _get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		val = n->val.getRealObject();
		return true;
	}
	return false;
}
bool rabbit::Table::newSlot(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val) const
{
	assert(key.isNull() == false);
	rabbit::Hash h = HashObj(key) & (_numofnodes - 1);
	_HashNode *n = _get(key, h);
	if (n) {
		n->val = val;
		return false;
	}
	_HashNode *mp = &_nodes[h];
	n = mp;


	//key not found I'll insert it
	//main pos is not free

	if(mp->key.isNull() == false) {
		n = _firstfree;  /* get a free place */
		rabbit::Hash mph = HashObj(mp->key) & (_numofnodes - 1);
		_HashNode *othern;  /* main position of colliding node */

		if (mp > n && (othern = &_nodes[mph]) != mp){
			/* yes; move colliding node into free position */
			while (othern->next != mp){
				assert(othern->next != NULL);
				othern = othern->next;  /* find previous */
			}
			othern->next = n;  /* redo the chain with `n' in place of `mp' */
			n->key = mp->key;
			n->val = mp->val;/* copy colliding node into free pos. (mp->next also goes) */
			n->next = mp->next;
			mp->key.Null();
			mp->val.Null();
			mp->next = NULL;  /* now `mp' is free */
		} else{
			/* new node will go into free position */
			n->next = mp->next;  /* chain new position */
			mp->next = n;
			mp = n;
		}
	}
	mp->key = key;

	for (;;) {
		/* correct `firstfree' */
		if (    _firstfree->key.isNull() == true
		     && _firstfree->next == NULL) {
			mp->val = val;
			_usednodes++;
			return true;  /* OK; table still has a free place */
		} else if (_firstfree == _nodes) {
			break;  /* cannot decrement from here */
		} else {
			(_firstfree)--;
		}
	}
	Rehash(true);
	return newSlot(key, val);
}

int64_t rabbit::Table::next(bool getweakrefs,const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval) const
{
	int64_t idx = (int64_t)translateIndex(refpos);
	while (idx < _numofnodes) {
		if(_nodes[idx].key.isNull() == false) {
			//first found
			_HashNode &n = _nodes[idx];
			outkey = n.key;
			outval = getweakrefs?(rabbit::Object)n.val:n.val.getRealObject();
			//return idx for the next iteration
			return ++idx;
		}
		++idx;
	}
	//nothing to iterate anymore
	return -1;
}


bool rabbit::Table::set(const rabbit::ObjectPtr &key, const rabbit::ObjectPtr &val) const
{
	_HashNode *n = _get(key, HashObj(key) & (_numofnodes - 1));
	if (n) {
		n->val = val;
		return true;
	}
	return false;
}

void rabbit::Table::_clearNodes() const
{
	for(int64_t i = 0;i < _numofnodes; i++) { _HashNode &n = _nodes[i]; n.key.Null(); n.val.Null(); }
}

void rabbit::Table::finalize()
{
	_clearNodes();
	setDelegate(NULL);
}

void rabbit::Table::clear() const
{
	_clearNodes();
	_usednodes = 0;
	Rehash(true);
}


rabbit::Table* rabbit::Table::create(rabbit::SharedState *ss,int64_t ninitialsize)
{
	char* tmp = (char*)SQ_MALLOC(sizeof(rabbit::Table));
	rabbit::Table *newtable = new (tmp) rabbit::Table(ss, ninitialsize);
	newtable->_delegate = NULL;
	return newtable;
}

rabbit::Table::~Table() {
	setDelegate(NULL);
	for (int64_t i = 0; i < _numofnodes; i++) _nodes[i].~_HashNode();
	SQ_FREE(_nodes, _numofnodes * sizeof(_HashNode));
}

rabbit::Table::_HashNode* rabbit::Table::_get(const rabbit::ObjectPtr &key,rabbit::Hash hash) const{
	_HashNode *n = &_nodes[hash];
	do {
		if(    n->key.toRaw() == key.toRaw()
		    && n->key.getType() == key.getType()){
			return n;
		}
	} while((n = n->next));
	return NULL;
}

//for compiler use
bool rabbit::Table::getStr(const char* key,int64_t keylen,rabbit::ObjectPtr &val) const{
	rabbit::Hash hash = _hashstr(key,keylen);
	_HashNode *n = &_nodes[hash & (_numofnodes - 1)];
	_HashNode *res = NULL;
	do {
		if (    n->key.isString() == true
		     && strcmp(n->key.getStringValue(), key) == 0) {
			res = n;
			break;
		}
	} while((n = n->next));
	if (res) {
		val = res->val.getRealObject();
		return true;
	}
	return false;
}

int64_t rabbit::Table::countUsed() const {
	return _usednodes;
}

void rabbit::Table::release() {
	ETK_DELETE(Table, this);
}
