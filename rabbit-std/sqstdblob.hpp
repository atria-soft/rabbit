/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

SQUIRREL_API SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size);
SQUIRREL_API SQRESULT sqstd_getblob(HSQUIRRELVM v,SQInteger idx,SQUserPointer *ptr);
SQUIRREL_API SQInteger sqstd_getblobsize(HSQUIRRELVM v,SQInteger idx);

SQUIRREL_API SQRESULT sqstd_register_bloblib(HSQUIRRELVM v);

