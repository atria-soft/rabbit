/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

struct SQInstance;

struct SQClassMember {
	rabbit::ObjectPtr val;
	rabbit::ObjectPtr attrs;
	void Null() {
		val.Null();
		attrs.Null();
	}
};

typedef etk::Vector<SQClassMember> SQClassMemberVec;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((int64_t)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((int64_t)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

struct SQClass : public rabbit::RefCounted
{
	SQClass(SQSharedState *ss,SQClass *base);
public:
	static SQClass* create(SQSharedState *ss,SQClass *base) {
		SQClass *newclass = (SQClass *)SQ_MALLOC(sizeof(SQClass));
		new (newclass) SQClass(ss, base);
		return newclass;
	}
	~SQClass();
	bool newSlot(SQSharedState *ss, const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val,bool bstatic);
	bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val) {
		if(_members->get(key,val)) {
			if(_isfield(val)) {
				rabbit::ObjectPtr &o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			}
			else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool getConstructor(rabbit::ObjectPtr &ctor)
	{
		if(_constructoridx != -1) {
			ctor = _methods[_constructoridx].val;
			return true;
		}
		return false;
	}
	bool setAttributes(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val);
	bool getAttributes(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &outval);
	void lock() { _locked = true; if(_base) _base->lock(); }
	void release() {
		if (_hook) { _hook(_typetag,0);}
		sq_delete(this, SQClass);
	}
	void finalize();
	int64_t next(const rabbit::ObjectPtr &refpos, rabbit::ObjectPtr &outkey, rabbit::ObjectPtr &outval);
	SQInstance *createInstance();
	SQTable *_members;
	SQClass *_base;
	SQClassMemberVec _defaultvalues;
	SQClassMemberVec _methods;
	rabbit::ObjectPtr _metamethods[MT_LAST];
	rabbit::ObjectPtr _attributes;
	rabbit::UserPointer _typetag;
	SQRELEASEHOOK _hook;
	bool _locked;
	int64_t _constructoridx;
	int64_t _udsize;
};

#define calcinstancesize(_theclass_) \
	(_theclass_->_udsize + sq_aligning(sizeof(SQInstance) +  (sizeof(rabbit::ObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0))))

struct SQInstance : public rabbit::Delegable
{
	void init(SQSharedState *ss);
	SQInstance(SQSharedState *ss, SQClass *c, int64_t memsize);
	SQInstance(SQSharedState *ss, SQInstance *c, int64_t memsize);
public:
	static SQInstance* create(SQSharedState *ss,SQClass *theclass) {

		int64_t size = calcinstancesize(theclass);
		SQInstance *newinst = (SQInstance *)SQ_MALLOC(size);
		new (newinst) SQInstance(ss, theclass,size);
		if(theclass->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
		}
		return newinst;
	}
	SQInstance *clone(SQSharedState *ss)
	{
		int64_t size = calcinstancesize(_class);
		SQInstance *newinst = (SQInstance *)SQ_MALLOC(size);
		new (newinst) SQInstance(ss, this,size);
		if(_class->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - _class->_udsize);
		}
		return newinst;
	}
	~SQInstance();
	bool get(const rabbit::ObjectPtr &key,rabbit::ObjectPtr &val)  {
		if(_class->_members->get(key,val)) {
			if(_isfield(val)) {
				rabbit::ObjectPtr &o = _values[_member_idx(val)];
				val = _realval(o);
			}
			else {
				val = _class->_methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool set(const rabbit::ObjectPtr &key,const rabbit::ObjectPtr &val) {
		rabbit::ObjectPtr idx;
		if(_class->_members->get(key,idx) && _isfield(idx)) {
			_values[_member_idx(idx)] = val;
			return true;
		}
		return false;
	}
	void release() {
		_uiRef++;
		if (_hook) { _hook(_userpointer,0);}
		_uiRef--;
		if(_uiRef > 0) return;
		int64_t size = _memsize;
		this->~SQInstance();
		SQ_FREE(this, size);
	}
	void finalize();
	bool instanceOf(SQClass *trg);
	bool getMetaMethod(rabbit::VirtualMachine *v,rabbit::MetaMethod mm,rabbit::ObjectPtr &res);

	SQClass *_class;
	rabbit::UserPointer _userpointer;
	SQRELEASEHOOK _hook;
	int64_t _memsize;
	rabbit::ObjectPtr _values[1];
};

