/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	class UserData : public SQDelegable {
		public:
			UserData(SQSharedState *ss) {
				_delegate = 0;
				m_hook = NULL;
			}
			~UserData() {
				SetDelegate(NULL);
			}
			static UserData* Create(SQSharedState *ss, int64_t size) {
				UserData* ud = (UserData*)SQ_MALLOC(sq_aligning(sizeof(UserData))+size);
				new (ud) UserData(ss);
				ud->m_size = size;
				ud->m_typetag = 0;
				return ud;
			}
			void Release() {
				if (m_hook) {
					m_hook((SQUserPointer)sq_aligning(this + 1),m_size);
				}
				int64_t tsize = m_size;
				this->~UserData();
				SQ_FREE(this, sq_aligning(sizeof(UserData)) + tsize);
			}
			const int64_t& getSize() const {
				return m_size;
			}
			const SQUserPointer& getTypeTag() const {
				return m_typetag;
			}
			void setTypeTag(const SQUserPointer& _value) {
				m_typetag = _value;
			}
			const SQRELEASEHOOK& getHook() const {
				return m_hook;
			}
			void setHook(const SQRELEASEHOOK& _value) {
				m_hook = _value;
			}
		protected:
			int64_t m_size;
			SQRELEASEHOOK m_hook;
			SQUserPointer m_typetag;
	};
}
