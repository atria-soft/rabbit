/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/Outer.hpp>
#include <etk/Allocator.hpp>
#include <rabbit/squtils.hpp>


rabbit::Outer::Outer(rabbit::SharedState *ss, rabbit::ObjectPtr *outer){
	_valptr = outer;
	_next = NULL;
}

rabbit::Outer* rabbit::Outer::create(rabbit::SharedState *ss, rabbit::ObjectPtr *outer) {
	rabbit::Outer *nc  = (rabbit::Outer*)SQ_MALLOC(sizeof(rabbit::Outer));
	new ((char*)nc) rabbit::Outer(ss, outer);
	return nc;
}

rabbit::Outer::~Outer() {
	
}

void rabbit::Outer::release() {
	this->~Outer();
	sq_vm_free(this,sizeof(rabbit::Outer));
}