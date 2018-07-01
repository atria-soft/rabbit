/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/UserData.hpp>
#include <etk/Allocator.hpp>

rabbit::UserData::UserData(rabbit::SharedState *ss) {
	_delegate = 0;
	m_hook = NULL;
}
rabbit::UserData::~UserData() {
	setDelegate(NULL);
}

rabbit::UserData* rabbit::UserData::create(rabbit::SharedState *ss, int64_t size) {
	UserData* ud = (UserData*)SQ_MALLOC(sq_aligning(sizeof(UserData))+size);
	new ((char*)ud) UserData(ss);
	ud->m_size = size;
	ud->m_typetag = 0;
	return ud;
}

void rabbit::UserData::release() {
	if (m_hook) {
		m_hook((rabbit::UserPointer)sq_aligning(this + 1),m_size);
	}
	int64_t tsize = m_size;
	this->~UserData();
	SQ_FREE(this, sq_aligning(sizeof(UserData)) + tsize);
}

const int64_t& rabbit::UserData::getsize() const {
	return m_size;
}

const rabbit::UserPointer& rabbit::UserData::getTypeTag() const {
	return m_typetag;
}

void rabbit::UserData::setTypeTag(const rabbit::UserPointer& _value) {
	m_typetag = _value;
}

const SQRELEASEHOOK& rabbit::UserData::getHook() const {
	return m_hook;
}

void rabbit::UserData::setHook(const SQRELEASEHOOK& _value) {
	m_hook = _value;
}
