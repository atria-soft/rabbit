/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	
	class LocalVarInfo {
		public:
			LocalVarInfo():_start_op(0),_end_op(0),_pos(0){}
			LocalVarInfo(const LocalVarInfo &lvi) {
				_name=lvi._name;
				_start_op=lvi._start_op;
				_end_op=lvi._end_op;
				_pos=lvi._pos;
			}
			rabbit::ObjectPtr _name;
			uint64_t _start_op;
			uint64_t _end_op;
			uint64_t _pos;
	};
}
