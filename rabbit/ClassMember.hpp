/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/sqconfig.hpp>

#include <rabbit/ObjectPtr.hpp>



#define MEMBER_TYPE_METHOD 0x01000000
#define MEMBER_TYPE_FIELD 0x02000000

#define _ismethod(o) (o.toInteger()&MEMBER_TYPE_METHOD)
#define _isfield(o) (o.toInteger()&MEMBER_TYPE_FIELD)
#define _make_method_idx(i) ((int64_t)(MEMBER_TYPE_METHOD|i))
#define _make_field_idx(i) ((int64_t)(MEMBER_TYPE_FIELD|i))
#define _member_type(o) (o.toInteger()&0xFF000000)
#define _member_idx(o) (o.toInteger()&0x00FFFFFF)

namespace rabbit {
	class ClassMember {
		public:
			rabbit::ObjectPtr val;
			rabbit::ObjectPtr attrs;
			void Null() {
				val.Null();
				attrs.Null();
			}
	};
}
