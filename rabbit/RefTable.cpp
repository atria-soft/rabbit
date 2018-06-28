/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/RefTable.hpp>
#include <rabbit/Table.hpp>

#include <rabbit/squtils.hpp>

rabbit::RefTable::RefTable()
{
	allocNodes(4);
}

void rabbit::RefTable::finalize()
{
	RefNode *nodes = _nodes;
	for(uint64_t n = 0; n < _numofslots; n++) {
		nodes->obj.Null();
		nodes++;
	}
}

rabbit::RefTable::~RefTable()
{
	SQ_FREE(_buckets,(_numofslots * sizeof(RefNode *)) + (_numofslots * sizeof(RefNode)));
}


void rabbit::RefTable::addRef(rabbit::Object &obj)
{
	rabbit::Hash mainpos;
	RefNode *prev;
	RefNode *ref = get(obj,mainpos,&prev,true);
	ref->refs++;
}

uint64_t rabbit::RefTable::getRefCount(rabbit::Object &obj)
{
	rabbit::Hash mainpos;
	RefNode *prev;
	RefNode *ref = get(obj,mainpos,&prev,true);
	return ref->refs;
}


rabbit::Bool rabbit::RefTable::release(rabbit::Object &obj)
{
	rabbit::Hash mainpos;
	RefNode *prev;
	RefNode *ref = get(obj,mainpos,&prev,false);
	if(ref) {
		if(--ref->refs == 0) {
			rabbit::ObjectPtr o = ref->obj;
			if(prev) {
				prev->next = ref->next;
			}
			else {
				_buckets[mainpos] = ref->next;
			}
			ref->next = _freelist;
			_freelist = ref;
			_slotused--;
			ref->obj.Null();
			//<<FIXME>>test for shrink?
			return SQTrue;
		}
	}
	else {
		assert(0);
	}
	return SQFalse;
}

void rabbit::RefTable::resize(uint64_t size)
{
	RefNode **oldbucks = _buckets;
	RefNode *t = _nodes;
	uint64_t oldnumofslots = _numofslots;
	allocNodes(size);
	//rehash
	uint64_t nfound = 0;
	for(uint64_t n = 0; n < oldnumofslots; n++) {
		if(sq_type(t->obj) != rabbit::OT_NULL) {
			//add back;
			assert(t->refs != 0);
			RefNode *nn = add(rabbit::HashObj(t->obj)&(_numofslots-1),t->obj);
			nn->refs = t->refs;
			t->obj.Null();
			nfound++;
		}
		t++;
	}
	assert(nfound == oldnumofslots);
	SQ_FREE(oldbucks,(oldnumofslots * sizeof(RefNode *)) + (oldnumofslots * sizeof(RefNode)));
}

rabbit::RefTable::RefNode* rabbit::RefTable::add(rabbit::Hash mainpos, rabbit::Object &obj)
{
	RefNode *t = _buckets[mainpos];
	RefNode *newnode = _freelist;
	newnode->obj = obj;
	_buckets[mainpos] = newnode;
	_freelist = _freelist->next;
	newnode->next = t;
	assert(newnode->refs == 0);
	_slotused++;
	return newnode;
}

rabbit::RefTable::RefNode* rabbit::RefTable::get(rabbit::Object &obj,rabbit::Hash &mainpos,RefNode **prev,bool addIfNeeded)
{
	RefNode *ref;
	mainpos = rabbit::HashObj(obj)&(_numofslots-1);
	*prev = NULL;
	for (ref = _buckets[mainpos]; ref; ) {
		if(_rawval(ref->obj) == _rawval(obj) && sq_type(ref->obj) == sq_type(obj))
			break;
		*prev = ref;
		ref = ref->next;
	}
	if(ref == NULL && addIfNeeded) {
		if(_numofslots == _slotused) {
			assert(_freelist == 0);
			resize(_numofslots*2);
			mainpos = rabbit::HashObj(obj)&(_numofslots-1);
		}
		ref = add(mainpos,obj);
	}
	return ref;
}

void rabbit::RefTable::allocNodes(uint64_t size)
{
	RefNode **bucks;
	RefNode *nodes;
	bucks = (RefNode **)SQ_MALLOC((size * sizeof(RefNode *)) + (size * sizeof(RefNode)));
	nodes = (RefNode *)&bucks[size];
	RefNode *temp = nodes;
	uint64_t n;
	for(n = 0; n < size - 1; n++) {
		bucks[n] = NULL;
		temp->refs = 0;
		new (&temp->obj) rabbit::ObjectPtr;
		temp->next = temp+1;
		temp++;
	}
	bucks[n] = NULL;
	temp->refs = 0;
	new (&temp->obj) rabbit::ObjectPtr;
	temp->next = NULL;
	_freelist = nodes;
	_nodes = nodes;
	_buckets = bucks;
	_slotused = 0;
	_numofslots = size;
}