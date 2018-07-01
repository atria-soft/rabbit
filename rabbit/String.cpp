/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/String.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/ObjectPtr.hpp>
#include <rabbit/StringTable.hpp>


rabbit::Hash rabbit::_hashstr(const char *s, size_t l) {
	rabbit::Hash h = (rabbit::Hash)l;  /* seed */
	size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
	for (; l>=step; l-=step)
		h = h ^ ((h<<5)+(h>>2)+(unsigned short)*(s++));
	return h;
}


rabbit::String *rabbit::String::create(rabbit::SharedState *ss,const char *s,int64_t len)
{
	rabbit::String *str=ADD_STRING(ss,s,len);
	return str;
}

void rabbit::String::release()
{
	REMOVE_STRING(_sharedstate,this);
}

int64_t rabbit::String::next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval)
{
	int64_t idx = (int64_t)translateIndex(refpos);
	while(idx < _len){
		outkey = (int64_t)idx;
		outval = (int64_t)((uint64_t)_val[idx]);
		//return idx for the next iteration
		return ++idx;
	}
	//nothing to iterate anymore
	return -1;
}

