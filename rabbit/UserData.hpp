/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once
#include <etk/types.hpp>
#include <rabbit/Delegable.hpp>

namespace rabbit {
	class UserData : public rabbit::Delegable {
		public:
			UserData(rabbit::SharedState *ss);
			~UserData();
			static UserData* create(rabbit::SharedState *ss, int64_t size);
			void release();
			const int64_t& getsize() const;
			const rabbit::UserPointer& getTypeTag() const;
			void setTypeTag(const rabbit::UserPointer& _value);
			const SQRELEASEHOOK& getHook() const;
			void setHook(const SQRELEASEHOOK& _value);
		protected:
			int64_t m_size;
			SQRELEASEHOOK m_hook;
			rabbit::UserPointer m_typetag;
	};
}
