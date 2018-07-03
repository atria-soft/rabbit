/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/RefCounted.hpp>
#include <rabbit/ObjectPtr.hpp>
#include <etk/Vector.hpp>

namespace rabbit {
	class SharedState;
	class Array : public rabbit::RefCounted {
		private:
			Array(rabbit::SharedState* _ss,
			      int64_t _nsize);
			~Array();
		public:
			// TODO : remove this ETK_ALLOC can do it natively ...
			static Array* create(rabbit::SharedState* _ss,
			                     int64_t _ninitialsize);
			void finalize();
			bool get(const int64_t _nidx,
			         rabbit::ObjectPtr& _val) const;
			bool set(const int64_t _nidx,const rabbit::ObjectPtr& _val) const;
			int64_t next(const rabbit::ObjectPtr& _refpos,
			             rabbit::ObjectPtr& _outkey,
			             rabbit::ObjectPtr& _outval);
			Array* clone() const;
			int64_t size() const;
			void resize(int64_t _size);
			void resize(int64_t _size,
			            rabbit::ObjectPtr& _fill);
			void reserve(int64_t _size);
			void append(const rabbit::Object& _o);
			void extend(const Array* _a);
			rabbit::ObjectPtr &top();
			void pop();
			bool insert(int64_t _idx,const rabbit::Object& _val);
			void shrinkIfNeeded();
			bool remove(int64_t _idx);
			void release();
			rabbit::ObjectPtr& operator[] (const size_t _pos);
			const rabbit::ObjectPtr& operator[] (const size_t _pos) const;
		private:
			mutable etk::Vector<rabbit::ObjectPtr> m_data;
	};
}

