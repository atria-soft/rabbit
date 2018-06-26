/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

RABBIT_API SQUserPointer sqstd_createblob(HRABBITVM v, SQInteger size);
RABBIT_API SQRESULT sqstd_getblob(HRABBITVM v,SQInteger idx,SQUserPointer *ptr);
RABBIT_API SQInteger sqstd_getblobsize(HRABBITVM v,SQInteger idx);

RABBIT_API SQRESULT sqstd_register_bloblib(HRABBITVM v);

