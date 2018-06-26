/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

struct SQArray : public SQRefCounted
{
private:
	SQArray(SQSharedState *ss,int64_t nsize) {
		_values.resize(nsize);
	}
	~SQArray()
	{
	}
public:
	static SQArray* Create(SQSharedState *ss,int64_t nInitialSize){
		SQArray *newarray=(SQArray*)SQ_MALLOC(sizeof(SQArray));
		new (newarray) SQArray(ss,nInitialSize);
		return newarray;
	}
	void Finalize(){
		_values.resize(0);
	}
	bool Get(const int64_t nidx,SQObjectPtr &val)
	{
		if(nidx>=0 && nidx<(int64_t)_values.size()){
			SQObjectPtr &o = _values[nidx];
			val = _realval(o);
			return true;
		}
		else return false;
	}
	bool Set(const int64_t nidx,const SQObjectPtr &val)
	{
		if(nidx>=0 && nidx<(int64_t)_values.size()){
			_values[nidx]=val;
			return true;
		}
		else return false;
	}
	int64_t Next(const SQObjectPtr &refpos,SQObjectPtr &outkey,SQObjectPtr &outval)
	{
		uint64_t idx=TranslateIndex(refpos);
		while(idx<_values.size()){
			//first found
			outkey=(int64_t)idx;
			SQObjectPtr &o = _values[idx];
			outval = _realval(o);
			//return idx for the next iteration
			return ++idx;
		}
		//nothing to iterate anymore
		return -1;
	}
	SQArray *Clone(){SQArray *anew=Create(NULL,0); anew->_values.copy(_values); return anew; }
	int64_t Size() const {return _values.size();}
	void Resize(int64_t size)
	{
		SQObjectPtr _null;
		Resize(size,_null);
	}
	void Resize(int64_t size,SQObjectPtr &fill) { _values.resize(size,fill); ShrinkIfNeeded(); }
	void Reserve(int64_t size) { _values.reserve(size); }
	void Append(const SQObject &o){_values.push_back(o);}
	void Extend(const SQArray *a);
	SQObjectPtr &Top(){return _values.top();}
	void Pop(){_values.pop_back(); ShrinkIfNeeded(); }
	bool Insert(int64_t idx,const SQObject &val){
		if(idx < 0 || idx > (int64_t)_values.size())
			return false;
		_values.insert(idx,val);
		return true;
	}
	void ShrinkIfNeeded() {
		if(_values.size() <= _values.capacity()>>2) //shrink the array
			_values.shrinktofit();
	}
	bool Remove(int64_t idx){
		if(idx < 0 || idx >= (int64_t)_values.size())
			return false;
		_values.remove(idx);
		ShrinkIfNeeded();
		return true;
	}
	void Release()
	{
		sq_delete(this,SQArray);
	}

	SQObjectPtrVec _values;
};


