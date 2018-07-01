/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once
#include <rabbit/sqopcodes.hpp>
#include <rabbit/squtils.hpp>
#include <rabbit/rabbit.hpp>
#include <rabbit/sqconfig.hpp>

namespace rabbit {
	
	class InstructionDesc {
		public:
			const char *name;
	};

	class Instruction {
		public:
			Instruction(){};
			Instruction(SQOpcode _op,int64_t a0=0,int64_t a1=0,int64_t a2=0,int64_t a3=0) {
				op = (unsigned char)_op;
				_arg0 = (unsigned char)a0;
				_arg1 = (int32_t)a1;
				_arg2 = (unsigned char)a2;
				_arg3 = (unsigned char)a3;
			}
		
			int32_t _arg1;
			unsigned char op;
			unsigned char _arg0;
			unsigned char _arg2;
			unsigned char _arg3;
	};

}
