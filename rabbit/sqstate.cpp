/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/sqpcheader.hpp>
#include <rabbit/sqopcodes.hpp>
#include <rabbit/VirtualMachine.hpp>
#include <rabbit/sqfuncproto.hpp>
#include <rabbit/sqclosure.hpp>
#include <rabbit/sqstring.hpp>
#include <rabbit/sqtable.hpp>
#include <rabbit/Array.hpp>
#include <rabbit/UserData.hpp>


SQSharedState::SQSharedState()
{
	_compilererrorhandler = NULL;
	_printfunc = NULL;
	_errorfunc = NULL;
	_debuginfo = false;
	_notifyallexceptions = false;
	_foreignptr = NULL;
	_releasehook = NULL;
}

#define newsysstring(s) {   \
	_systemstrings->pushBack(SQString::create(this,s));	\
	}

#define newmetamethod(s) {  \
	_metamethods->pushBack(SQString::create(this,s));  \
	_table(_metamethodsmap)->newSlot(_metamethods->back(),(int64_t)(_metamethods->size()-1)); \
	}

bool compileTypemask(etk::Vector<int64_t> &res,const rabbit::Char *typemask)
{
	int64_t i = 0;
	int64_t mask = 0;
	while(typemask[i] != 0) {
		switch(typemask[i]) {
			case 'o': mask |= _RT_NULL; break;
			case 'i': mask |= _RT_INTEGER; break;
			case 'f': mask |= _RT_FLOAT; break;
			case 'n': mask |= (_RT_FLOAT | _RT_INTEGER); break;
			case 's': mask |= _RT_STRING; break;
			case 't': mask |= _RT_TABLE; break;
			case 'a': mask |= _RT_ARRAY; break;
			case 'u': mask |= _RT_USERDATA; break;
			case 'c': mask |= (_RT_CLOSURE | _RT_NATIVECLOSURE); break;
			case 'b': mask |= _RT_BOOL; break;
			case 'g': mask |= _RT_GENERATOR; break;
			case 'p': mask |= _RT_USERPOINTER; break;
			case 'v': mask |= _RT_THREAD; break;
			case 'x': mask |= _RT_INSTANCE; break;
			case 'y': mask |= _RT_CLASS; break;
			case 'r': mask |= _RT_WEAKREF; break;
			case '.': mask = -1; res.pushBack(mask); i++; mask = 0; continue;
			case ' ': i++; continue; //ignores spaces
			default:
				return false;
		}
		i++;
		if(typemask[i] == '|') {
			i++;
			if(typemask[i] == 0)
				return false;
			continue;
		}
		res.pushBack(mask);
		mask = 0;

	}
	return true;
}

SQTable *createDefaultDelegate(SQSharedState *ss,const rabbit::RegFunction *funcz)
{
	int64_t i=0;
	SQTable *t=SQTable::create(ss,0);
	while(funcz[i].name!=0){
		SQNativeClosure *nc = SQNativeClosure::create(ss,funcz[i].f,0);
		nc->_nparamscheck = funcz[i].nparamscheck;
		nc->_name = SQString::create(ss,funcz[i].name);
		if(funcz[i].typemask && !compileTypemask(nc->_typecheck,funcz[i].typemask))
			return NULL;
		t->newSlot(SQString::create(ss,funcz[i].name),nc);
		i++;
	}
	return t;
}

void SQSharedState::init()
{
	_scratchpad=NULL;
	_scratchpadsize=0;
	_stringtable = (SQStringTable*)SQ_MALLOC(sizeof(SQStringTable));
	new (_stringtable) SQStringTable(this);
	sq_new(_metamethods,etk::Vector<rabbit::ObjectPtr>);
	sq_new(_systemstrings,etk::Vector<rabbit::ObjectPtr>);
	sq_new(_types,etk::Vector<rabbit::ObjectPtr>);
	_metamethodsmap = SQTable::create(this,rabbit::MT_LAST-1);
	//adding type strings to avoid memory trashing
	//types names
	newsysstring(_SC("null"));
	newsysstring(_SC("table"));
	newsysstring(_SC("array"));
	newsysstring(_SC("closure"));
	newsysstring(_SC("string"));
	newsysstring(_SC("userdata"));
	newsysstring(_SC("integer"));
	newsysstring(_SC("float"));
	newsysstring(_SC("userpointer"));
	newsysstring(_SC("function"));
	newsysstring(_SC("generator"));
	newsysstring(_SC("thread"));
	newsysstring(_SC("class"));
	newsysstring(_SC("instance"));
	newsysstring(_SC("bool"));
	//meta methods
	newmetamethod(MM_ADD);
	newmetamethod(MM_SUB);
	newmetamethod(MM_MUL);
	newmetamethod(MM_DIV);
	newmetamethod(MM_UNM);
	newmetamethod(MM_MODULO);
	newmetamethod(MM_SET);
	newmetamethod(MM_GET);
	newmetamethod(MM_TYPEOF);
	newmetamethod(MM_NEXTI);
	newmetamethod(MM_CMP);
	newmetamethod(MM_CALL);
	newmetamethod(MM_CLONED);
	newmetamethod(MM_NEWSLOT);
	newmetamethod(MM_DELSLOT);
	newmetamethod(MM_TOSTRING);
	newmetamethod(MM_NEWMEMBER);
	newmetamethod(MM_INHERITED);

	_constructoridx = SQString::create(this,_SC("constructor"));
	_registry = SQTable::create(this,0);
	_consts = SQTable::create(this,0);
	_table_default_delegate = createDefaultDelegate(this,_table_default_delegate_funcz);
	_array_default_delegate = createDefaultDelegate(this,_array_default_delegate_funcz);
	_string_default_delegate = createDefaultDelegate(this,_string_default_delegate_funcz);
	_number_default_delegate = createDefaultDelegate(this,_number_default_delegate_funcz);
	_closure_default_delegate = createDefaultDelegate(this,_closure_default_delegate_funcz);
	_generator_default_delegate = createDefaultDelegate(this,_generator_default_delegate_funcz);
	_thread_default_delegate = createDefaultDelegate(this,_thread_default_delegate_funcz);
	_class_default_delegate = createDefaultDelegate(this,_class_default_delegate_funcz);
	_instance_default_delegate = createDefaultDelegate(this,_instance_default_delegate_funcz);
	_weakref_default_delegate = createDefaultDelegate(this,_weakref_default_delegate_funcz);
}

SQSharedState::~SQSharedState()
{
	if(_releasehook) { _releasehook(_foreignptr,0); _releasehook = NULL; }
	_constructoridx.Null();
	_table(_registry)->finalize();
	_table(_consts)->finalize();
	_table(_metamethodsmap)->finalize();
	_registry.Null();
	_consts.Null();
	_metamethodsmap.Null();
	while(!_systemstrings->empty()) {
		_systemstrings->back().Null();
		_systemstrings->popBack();
	}
	_thread(_root_vm)->finalize();
	_root_vm.Null();
	_table_default_delegate.Null();
	_array_default_delegate.Null();
	_string_default_delegate.Null();
	_number_default_delegate.Null();
	_closure_default_delegate.Null();
	_generator_default_delegate.Null();
	_thread_default_delegate.Null();
	_class_default_delegate.Null();
	_instance_default_delegate.Null();
	_weakref_default_delegate.Null();
	_refs_table.finalize();

	using tmpType = etk::Vector<rabbit::ObjectPtr>;
	sq_delete(_types, tmpType);
	sq_delete(_systemstrings, tmpType);
	sq_delete(_metamethods, tmpType);
	sq_delete(_stringtable, SQStringTable);
	if(_scratchpad)SQ_FREE(_scratchpad,_scratchpadsize);
}


int64_t SQSharedState::getMetaMethodIdxByName(const rabbit::ObjectPtr &name)
{
	if(sq_type(name) != rabbit::OT_STRING)
		return -1;
	rabbit::ObjectPtr ret;
	if(_table(_metamethodsmap)->get(name,ret)) {
		return _integer(ret);
	}
	return -1;
}


rabbit::Char* SQSharedState::getScratchPad(int64_t size)
{
	int64_t newsize;
	if(size>0) {
		if(_scratchpadsize < size) {
			newsize = size + (size>>1);
			_scratchpad = (rabbit::Char *)SQ_REALLOC(_scratchpad,_scratchpadsize,newsize);
			_scratchpadsize = newsize;

		}else if(_scratchpadsize >= (size<<5)) {
			newsize = _scratchpadsize >> 1;
			_scratchpad = (rabbit::Char *)SQ_REALLOC(_scratchpad,_scratchpadsize,newsize);
			_scratchpadsize = newsize;
		}
	}
	return _scratchpad;
}

RefTable::RefTable()
{
	allocNodes(4);
}

void RefTable::finalize()
{
	RefNode *nodes = _nodes;
	for(uint64_t n = 0; n < _numofslots; n++) {
		nodes->obj.Null();
		nodes++;
	}
}

RefTable::~RefTable()
{
	SQ_FREE(_buckets,(_numofslots * sizeof(RefNode *)) + (_numofslots * sizeof(RefNode)));
}


void RefTable::addRef(rabbit::Object &obj)
{
	SQHash mainpos;
	RefNode *prev;
	RefNode *ref = get(obj,mainpos,&prev,true);
	ref->refs++;
}

uint64_t RefTable::getRefCount(rabbit::Object &obj)
{
	 SQHash mainpos;
	 RefNode *prev;
	 RefNode *ref = get(obj,mainpos,&prev,true);
	 return ref->refs;
}


rabbit::Bool RefTable::release(rabbit::Object &obj)
{
	SQHash mainpos;
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

void RefTable::resize(uint64_t size)
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
			RefNode *nn = add(::HashObj(t->obj)&(_numofslots-1),t->obj);
			nn->refs = t->refs;
			t->obj.Null();
			nfound++;
		}
		t++;
	}
	assert(nfound == oldnumofslots);
	SQ_FREE(oldbucks,(oldnumofslots * sizeof(RefNode *)) + (oldnumofslots * sizeof(RefNode)));
}

RefTable::RefNode *RefTable::add(SQHash mainpos,rabbit::Object &obj)
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

RefTable::RefNode *RefTable::get(rabbit::Object &obj,SQHash &mainpos,RefNode **prev,bool addIfNeeded)
{
	RefNode *ref;
	mainpos = ::HashObj(obj)&(_numofslots-1);
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
			mainpos = ::HashObj(obj)&(_numofslots-1);
		}
		ref = add(mainpos,obj);
	}
	return ref;
}

void RefTable::allocNodes(uint64_t size)
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
//////////////////////////////////////////////////////////////////////////
//SQStringTable
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_lstring.c.html
*/

SQStringTable::SQStringTable(SQSharedState *ss)
{
	_sharedstate = ss;
	allocNodes(4);
	_slotused = 0;
}

SQStringTable::~SQStringTable()
{
	SQ_FREE(_strings,sizeof(SQString*)*_numofslots);
	_strings = NULL;
}

void SQStringTable::allocNodes(int64_t size)
{
	_numofslots = size;
	_strings = (SQString**)SQ_MALLOC(sizeof(SQString*)*_numofslots);
	memset(_strings,0,sizeof(SQString*)*_numofslots);
}

SQString *SQStringTable::add(const rabbit::Char *news,int64_t len)
{
	if(len<0)
		len = (int64_t)scstrlen(news);
	SQHash newhash = ::_hashstr(news,len);
	SQHash h = newhash&(_numofslots-1);
	SQString *s;
	for (s = _strings[h]; s; s = s->_next){
		if(s->_len == len && (!memcmp(news,s->_val,sq_rsl(len))))
			return s; //found
	}

	SQString *t = (SQString *)SQ_MALLOC(sq_rsl(len)+sizeof(SQString));
	new (t) SQString;
	t->_sharedstate = _sharedstate;
	memcpy(t->_val,news,sq_rsl(len));
	t->_val[len] = _SC('\0');
	t->_len = len;
	t->_hash = newhash;
	t->_next = _strings[h];
	_strings[h] = t;
	_slotused++;
	if (_slotused > _numofslots)  /* too crowded? */
		resize(_numofslots*2);
	return t;
}

void SQStringTable::resize(int64_t size)
{
	int64_t oldsize=_numofslots;
	SQString **oldtable=_strings;
	allocNodes(size);
	for (int64_t i=0; i<oldsize; i++){
		SQString *p = oldtable[i];
		while(p){
			SQString *next = p->_next;
			SQHash h = p->_hash&(_numofslots-1);
			p->_next = _strings[h];
			_strings[h] = p;
			p = next;
		}
	}
	SQ_FREE(oldtable,oldsize*sizeof(SQString*));
}

void SQStringTable::remove(SQString *bs)
{
	SQString *s;
	SQString *prev=NULL;
	SQHash h = bs->_hash&(_numofslots - 1);

	for (s = _strings[h]; s; ){
		if(s == bs){
			if(prev)
				prev->_next = s->_next;
			else
				_strings[h] = s->_next;
			_slotused--;
			int64_t slen = s->_len;
			s->~SQString();
			SQ_FREE(s,sizeof(SQString) + sq_rsl(slen));
			return;
		}
		prev = s;
		s = s->_next;
	}
	assert(0);//if this fail something is wrong
}
