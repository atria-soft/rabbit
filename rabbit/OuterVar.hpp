/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

namespace rabbit {
	enum OuterType {
		otLOCAL = 0,
		otOUTER = 1
	};

	class OuterVar {
		public:
			OuterVar(){}
			OuterVar(const ObjectPtr &name,const ObjectPtr &src, OuterType t) {
				_name = name;
				_src=src;
				_type=t;
			}
			OuterVar(const OuterVar &ov) {
				_type=ov._type;
				_src=ov._src;
				_name=ov._name;
			}
			rabbit::OuterType _type;
			rabbit::ObjectPtr _name;
			rabbit::ObjectPtr _src;
	};

}
