/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */

#include <rabbit/AutoDec.hpp>

rabbit::AutoDec::AutoDec(int64_t* _count) {
	m_countPointer = _count;
}

rabbit::AutoDec::~AutoDec() {
	(*m_countPointer)--;
}

