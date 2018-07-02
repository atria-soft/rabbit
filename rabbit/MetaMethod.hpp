/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/sqconfig.hpp>

namespace rabbit {
	enum MetaMethod {
		MT_ADD=0,
		MT_SUB=1,
		MT_MUL=2,
		MT_DIV=3,
		MT_UNM=4,
		MT_MODULO=5,
		MT_SET=6,
		MT_GET=7,
		MT_TYPEOF=8,
		MT_NEXTI=9,
		MT_CMP=10,
		MT_CALL=11,
		MT_CLONED=12,
		MT_NEWSLOT=13,
		MT_DELSLOT=14,
		MT_TOSTRING=15,
		MT_NEWMEMBER=16,
		MT_INHERITED=17,
		MT_LAST = 18
	};
}

#define MM_ADD	  "_add"
#define MM_SUB	  "_sub"
#define MM_MUL	  "_mul"
#define MM_DIV	  "_div"
#define MM_UNM	  "_unm"
#define MM_MODULO   "_modulo"
#define MM_SET	  "_set"
#define MM_GET	  "_get"
#define MM_TYPEOF   "_typeof"
#define MM_NEXTI	"_nexti"
#define MM_CMP	  "_cmp"
#define MM_CALL	 "_call"
#define MM_CLONED   "_cloned"
#define MM_NEWSLOT  "_newslot"
#define MM_DELSLOT  "_delslot"
#define MM_TOSTRING "_tostring"
#define MM_NEWMEMBER "_newmember"
#define MM_INHERITED "_inherited"
