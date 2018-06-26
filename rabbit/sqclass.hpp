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
	SQObjectPtr val;
	SQObjectPtr attrs;
	void Null() {
		val.Null();
		attrs.Null();
	}
};

typedef sqvector<SQClassMember> SQClassMemberVec;

#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (_integer(o)&MEMBER_TYPE_METHOD)
#define _isfield(o) (_integer(o)&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((int64_t)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((int64_t)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (_integer(o)&0xFF000000)
#define _member_idx(o) (_integer(o)&0x00FFFFFF)

struct SQClass : public SQRefCounted
{
	SQClass(SQSharedState *ss,SQClass *base);
public:
	static SQClass* Create(SQSharedState *ss,SQClass *base) {
		SQClass *newclass = (SQClass *)SQ_MALLOC(sizeof(SQClass));
		new (newclass) SQClass(ss, base);
		return newclass;
	}
	~SQClass();
	bool NewSlot(SQSharedState *ss, const SQObjectPtr &key,const SQObjectPtr &val,bool bstatic);
	bool Get(const SQObjectPtr &key,SQObjectPtr &val) {
		if(_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr &o = _defaultvalues[_member_idx(val)].val;
				val = _realval(o);
			}
			else {
				val = _methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool GetConstructor(SQObjectPtr &ctor)
	{
		if(_constructoridx != -1) {
			ctor = _methods[_constructoridx].val;
			return true;
		}
		return false;
	}
	bool SetAttributes(const SQObjectPtr &key,const SQObjectPtr &val);
	bool GetAttributes(const SQObjectPtr &key,SQObjectPtr &outval);
	void Lock() { _locked = true; if(_base) _base->Lock(); }
	void Release() {
		if (_hook) { _hook(_typetag,0);}
		sq_delete(this, SQClass);
	}
	void Finalize();
	int64_t Next(const SQObjectPtr &refpos, SQObjectPtr &outkey, SQObjectPtr &outval);
	SQInstance *CreateInstance();
	SQTable *_members;
	SQClass *_base;
	SQClassMemberVec _defaultvalues;
	SQClassMemberVec _methods;
	SQObjectPtr _metamethods[MT_LAST];
	SQObjectPtr _attributes;
	SQUserPointer _typetag;
	SQRELEASEHOOK _hook;
	bool _locked;
	int64_t _constructoridx;
	int64_t _udsize;
};

#define calcinstancesize(_theclass_) \
	(_theclass_->_udsize + sq_aligning(sizeof(SQInstance) +  (sizeof(SQObjectPtr)*(_theclass_->_defaultvalues.size()>0?_theclass_->_defaultvalues.size()-1:0))))

struct SQInstance : public SQDelegable
{
	void Init(SQSharedState *ss);
	SQInstance(SQSharedState *ss, SQClass *c, int64_t memsize);
	SQInstance(SQSharedState *ss, SQInstance *c, int64_t memsize);
public:
	static SQInstance* Create(SQSharedState *ss,SQClass *theclass) {

		int64_t size = calcinstancesize(theclass);
		SQInstance *newinst = (SQInstance *)SQ_MALLOC(size);
		new (newinst) SQInstance(ss, theclass,size);
		if(theclass->_udsize) {
			newinst->_userpointer = ((unsigned char *)newinst) + (size - theclass->_udsize);
		}
		return newinst;
	}
	SQInstance *Clone(SQSharedState *ss)
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
	bool Get(const SQObjectPtr &key,SQObjectPtr &val)  {
		if(_class->_members->Get(key,val)) {
			if(_isfield(val)) {
				SQObjectPtr &o = _values[_member_idx(val)];
				val = _realval(o);
			}
			else {
				val = _class->_methods[_member_idx(val)].val;
			}
			return true;
		}
		return false;
	}
	bool Set(const SQObjectPtr &key,const SQObjectPtr &val) {
		SQObjectPtr idx;
		if(_class->_members->Get(key,idx) && _isfield(idx)) {
			_values[_member_idx(idx)] = val;
			return true;
		}
		return false;
	}
	void Release() {
		_uiRef++;
		if (_hook) { _hook(_userpointer,0);}
		_uiRef--;
		if(_uiRef > 0) return;
		int64_t size = _memsize;
		this->~SQInstance();
		SQ_FREE(this, size);
	}
	void Finalize();
	bool InstanceOf(SQClass *trg);
	bool GetMetaMethod(SQVM *v,SQMetaMethod mm,SQObjectPtr &res);

	SQClass *_class;
	SQUserPointer _userpointer;
	SQRELEASEHOOK _hook;
	int64_t _memsize;
	SQObjectPtr _values[1];
};

