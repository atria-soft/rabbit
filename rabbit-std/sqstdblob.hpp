/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

RABBIT_API SQUserPointer sqstd_createblob(rabbit::VirtualMachine* v, int64_t size);
RABBIT_API SQRESULT sqstd_getblob(rabbit::VirtualMachine* v,int64_t idx,SQUserPointer *ptr);
RABBIT_API int64_t sqstd_getblobsize(rabbit::VirtualMachine* v,int64_t idx);

RABBIT_API SQRESULT sqstd_register_bloblib(rabbit::VirtualMachine* v);

