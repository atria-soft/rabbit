/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/StringTable.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/ObjectPtr.hpp>
#include <rabbit/StringTable.hpp>
#include <rabbit/squtils.hpp>




rabbit::StringTable::StringTable(rabbit::SharedState *ss)
{
	_sharedstate = ss;
	allocNodes(4);
	_slotused = 0;
}

rabbit::StringTable::~StringTable()
{
	SQ_FREE(_strings,sizeof(rabbit::String*)*_numofslots);
	_strings = NULL;
}

void rabbit::StringTable::allocNodes(int64_t size)
{
	_numofslots = size;
	_strings = (rabbit::String**)SQ_MALLOC(sizeof(rabbit::String*)*_numofslots);
	memset(_strings,0,sizeof(rabbit::String*)*_numofslots);
}

rabbit::String *rabbit::StringTable::add(const char *news,int64_t len)
{
	if(len<0)
		len = (int64_t)strlen(news);
	rabbit::Hash newhash = _hashstr(news,len);
	rabbit::Hash h = newhash&(_numofslots-1);
	rabbit::String *s;
	for (s = _strings[h]; s; s = s->_next){
		if(s->_len == len && (!memcmp(news,s->_val,sq_rsl(len))))
			return s; //found
	}

	rabbit::String *t = (rabbit::String *)SQ_MALLOC(sq_rsl(len)+sizeof(rabbit::String));
	new ((char*)t) rabbit::String;
	t->_sharedstate = _sharedstate;
	memcpy(t->_val,news,sq_rsl(len));
	t->_val[len] = '\0';
	t->_len = len;
	t->_hash = newhash;
	t->_next = _strings[h];
	_strings[h] = t;
	_slotused++;
	if (_slotused > _numofslots)  /* too crowded? */
		resize(_numofslots*2);
	return t;
}

void rabbit::StringTable::resize(int64_t size)
{
	int64_t oldsize=_numofslots;
	rabbit::String **oldtable=_strings;
	allocNodes(size);
	for (int64_t i=0; i<oldsize; i++){
		rabbit::String *p = oldtable[i];
		while(p){
			rabbit::String *next = p->_next;
			rabbit::Hash h = p->_hash&(_numofslots-1);
			p->_next = _strings[h];
			_strings[h] = p;
			p = next;
		}
	}
	SQ_FREE(oldtable,oldsize*sizeof(rabbit::String*));
}

void rabbit::StringTable::remove(rabbit::String *bs)
{
	rabbit::String *s;
	rabbit::String *prev=NULL;
	rabbit::Hash h = bs->_hash&(_numofslots - 1);

	for (s = _strings[h]; s; ){
		if(s == bs){
			if(prev)
				prev->_next = s->_next;
			else
				_strings[h] = s->_next;
			_slotused--;
			int64_t slen = s->_len;
			s->~String();
			SQ_FREE(s,sizeof(rabbit::String) + sq_rsl(slen));
			return;
		}
		prev = s;
		s = s->_next;
	}
	assert(0);//if this fail something is wrong
}
