/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

inline SQHash _hashstr (const rabbit::Char *s, size_t l)
{
		SQHash h = (SQHash)l;  /* seed */
		size_t step = (l>>5)|1;  /* if string is too long, don't hash all its chars */
		for (; l>=step; l-=step)
			h = h ^ ((h<<5)+(h>>2)+(unsigned short)*(s++));
		return h;
}

struct SQString : public rabbit::RefCounted
{
	SQString(){}
	~SQString(){}
public:
	static SQString *create(SQSharedState *ss, const rabbit::Char *, int64_t len = -1 );
	int64_t next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval);
	void release();
	SQSharedState *_sharedstate;
	SQString *_next; //chain for the string table
	int64_t _len;
	SQHash _hash;
	rabbit::Char _val[1];
};



