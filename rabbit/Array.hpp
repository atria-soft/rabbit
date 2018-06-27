/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	class Array : public rabbit::RefCounted
	{
		private:
			Array(SQSharedState* _ss,
			      int64_t _nsize) {
				m_data.resize(_nsize);
			}
			~Array() {
				// TODO: Clean DATA ...
			}
		public:
			// TODO : remove this ETK_ALLOC can do it natively ...
			static Array* create(SQSharedState* _ss,
			                     int64_t _ninitialsize) {
				Array *newarray=(Array*)SQ_MALLOC(sizeof(Array));
				new (newarray) Array(_ss, _ninitialsize);
				return newarray;
			}
			void finalize() {
				m_data.resize(0);
			}
			bool get(const int64_t _nidx,
			         SQObjectPtr& _val) {
				if(    _nidx >= 0
				    && _nidx < (int64_t)m_data.size()){
					SQObjectPtr &o = m_data[_nidx];
					_val = _realval(o);
					return true;
				}
				return false;
			}
			bool set(const int64_t _nidx,const SQObjectPtr& _val) {
				if(_nidx>=0 && _nidx<(int64_t)m_data.size()){
					m_data[_nidx] = _val;
					return true;
				}
				return false;
			}
			int64_t next(const SQObjectPtr& _refpos,
			             SQObjectPtr& _outkey,
			             SQObjectPtr& _outval) {
				uint64_t idx=translateIndex(_refpos);
				while(idx<m_data.size()){
					//first found
					_outkey=(int64_t)idx;
					SQObjectPtr& o = m_data[idx];
					_outval = _realval(o);
					//return idx for the next iteration
					return ++idx;
				}
				//nothing to iterate anymore
				return -1;
			}
			Array* clone() {
				Array *anew = create(NULL,0);
				anew->m_data = m_data;
				return anew;
			}
			int64_t size() const {
				return m_data.size();
			}
			void resize(int64_t _size) {
				SQObjectPtr empty;
				resize(_size, empty);
			}
			void resize(int64_t _size,
			            SQObjectPtr& _fill) {
				m_data.resize(_size, _fill);
				shrinkIfNeeded();
			}
			void reserve(int64_t _size) {
				m_data.reserve(_size);
			}
			void append(const SQObject& _o) {
				m_data.pushBack(_o);
			}
			void extend(const Array* _a);
			SQObjectPtr &top(){
				return m_data.back();
			}
			void pop() {
				m_data.popBack();
				shrinkIfNeeded();
			}
			bool insert(int64_t _idx,const SQObject& _val) {
				if(    _idx < 0
				    || _idx > (int64_t)m_data.size()) {
					return false;
				}
				m_data.insert(_idx, _val);
				return true;
			}
			void shrinkIfNeeded() {
				// TODO: Check this. No real need with etk ==> automatic ...
				/*
				if(m_data.size() <= m_data.capacity()>>2) {
					//shrink the array
					m_data.shrinktofit();
				}
				*/
			}
			bool remove(int64_t _idx) {
				if(    _idx < 0
				    || _idx >= (int64_t)m_data.size()) {
					return false;
				}
				m_data.remove(_idx);
				shrinkIfNeeded();
				return true;
			}
			void release() {
				sq_delete(this, Array);
			}
			SQObjectPtr& operator[] (const size_t _pos) {
				return m_data[_pos];
			}
			const SQObjectPtr& operator[] (const size_t _pos) const {
				return m_data[_pos];
			}
		private:
			SQObjectPtrVec m_data;
	};
}

