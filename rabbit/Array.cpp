/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Array.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/WeakRef.hpp>



void rabbit::Array::extend(const rabbit::Array *a){
	int64_t xlen = a->size();
	// TODO: check this condition...
	if(xlen) {
		for(int64_t iii=0;iii<xlen;++iii) {
			append((*a)[iii]);
		}
	}
}

rabbit::Array::Array(rabbit::SharedState* _ss, int64_t _nsize) {
	m_data.resize(_nsize);
}
rabbit::Array::~Array() {
	// TODO: Clean DATA ...
}
// TODO : remove this ETK_ALLOC can do it natively ...
rabbit::Array* rabbit::Array::create(rabbit::SharedState* _ss,
                     int64_t _ninitialsize) {
	Array *newarray=(Array*)SQ_MALLOC(sizeof(Array));
	new ((char*)newarray) Array(_ss, _ninitialsize);
	return newarray;
}
void rabbit::Array::finalize() {
	m_data.resize(0);
}
bool rabbit::Array::get(const int64_t _nidx, rabbit::ObjectPtr& _val) const {
	if(    _nidx >= 0
	    && _nidx < (int64_t)m_data.size()){
		rabbit::ObjectPtr &o = m_data[_nidx];
		_val = _realval(o);
		return true;
	}
	return false;
}
bool rabbit::Array::set(const int64_t _nidx,const rabbit::ObjectPtr& _val) const {
	if(_nidx>=0 && _nidx<(int64_t)m_data.size()){
		m_data[_nidx] = _val;
		return true;
	}
	return false;
}
int64_t rabbit::Array::next(const rabbit::ObjectPtr& _refpos,
             rabbit::ObjectPtr& _outkey,
             rabbit::ObjectPtr& _outval) {
	uint64_t idx=translateIndex(_refpos);
	while(idx<m_data.size()){
		//first found
		_outkey=(int64_t)idx;
		rabbit::ObjectPtr& o = m_data[idx];
		_outval = _realval(o);
		//return idx for the next iteration
		return ++idx;
	}
	//nothing to iterate anymore
	return -1;
}
rabbit::Array* rabbit::Array::clone() const {
	Array *anew = create(NULL,0);
	anew->m_data = m_data;
	return anew;
}
int64_t rabbit::Array::size() const {
	return m_data.size();
}
void rabbit::Array::resize(int64_t _size) {
	rabbit::ObjectPtr empty;
	resize(_size, empty);
}
void rabbit::Array::resize(int64_t _size,
            rabbit::ObjectPtr& _fill) {
	m_data.resize(_size, _fill);
	shrinkIfNeeded();
}
void rabbit::Array::reserve(int64_t _size) {
	m_data.reserve(_size);
}
void rabbit::Array::append(const rabbit::Object& _o) {
	m_data.pushBack(_o);
}

rabbit::ObjectPtr& rabbit::Array::top(){
	return m_data.back();
}
void rabbit::Array::pop() {
	m_data.popBack();
	shrinkIfNeeded();
}
bool rabbit::Array::insert(int64_t _idx,const rabbit::Object& _val) {
	if(    _idx < 0
	    || _idx > (int64_t)m_data.size()) {
		return false;
	}
	m_data.insert(_idx, _val);
	return true;
}
void rabbit::Array::shrinkIfNeeded() {
	// TODO: Check this. No real need with etk ==> automatic ...
	/*
	if(m_data.size() <= m_data.capacity()>>2) {
		//shrink the array
		m_data.shrinktofit();
	}
	*/
}
bool rabbit::Array::remove(int64_t _idx) {
	if(    _idx < 0
	    || _idx >= (int64_t)m_data.size()) {
		return false;
	}
	m_data.remove(_idx);
	shrinkIfNeeded();
	return true;
}
void rabbit::Array::release() {
	sq_delete(this, Array);
}
rabbit::ObjectPtr& rabbit::Array::operator[] (const size_t _pos) {
	return m_data[_pos];
}
const rabbit::ObjectPtr& rabbit::Array::operator[] (const size_t _pos) const {
	return m_data[_pos];
}