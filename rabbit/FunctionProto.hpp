/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once
#include <rabbit/LocalVarInfo.hpp>
#include <rabbit/LineInfo.hpp>
#include <rabbit/OuterVar.hpp>
#include <rabbit/Instruction.hpp>
#include <rabbit/RefCounted.hpp>
#include <rabbit/rabbit.hpp>


namespace rabbit {
	
	#define _FUNC_SIZE(ni,nl,nparams,nfuncs,nouters,nlineinf,localinf,defparams) (sizeof(rabbit::FunctionProto) \
			+((ni-1)*sizeof(rabbit::Instruction))+(nl*sizeof(rabbit::ObjectPtr)) \
			+(nparams*sizeof(rabbit::ObjectPtr))+(nfuncs*sizeof(rabbit::ObjectPtr)) \
			+(nouters*sizeof(rabbit::OuterVar))+(nlineinf*sizeof(rabbit::LineInfo)) \
			+(localinf*sizeof(rabbit::LocalVarInfo))+(defparams*sizeof(int64_t)))
	
	
	class FunctionProto : public rabbit::RefCounted {
		private:
			FunctionProto(rabbit::SharedState *ss);
			~FunctionProto();
		
		public:
			static FunctionProto *create(rabbit::SharedState *ss,int64_t ninstructions,
				int64_t nliterals,int64_t nparameters,
				int64_t nfunctions,int64_t noutervalues,
				int64_t nlineinfos,int64_t nlocalvarinfos,int64_t ndefaultparams);
			void release();
		
			const rabbit::Char* getLocal(rabbit::VirtualMachine *v,uint64_t stackbase,uint64_t nseq,uint64_t nop);
			int64_t getLine(rabbit::Instruction *curr);
			bool save(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQWRITEFUNC write);
			static bool load(rabbit::VirtualMachine *v,rabbit::UserPointer up,SQREADFUNC read,rabbit::ObjectPtr &ret);
			rabbit::ObjectPtr _sourcename;
			rabbit::ObjectPtr _name;
			int64_t _stacksize;
			bool _bgenerator;
			int64_t _varparams;
		
			int64_t _nlocalvarinfos;
			rabbit::LocalVarInfo *_localvarinfos;
		
			int64_t _nlineinfos;
			rabbit::LineInfo *_lineinfos;
		
			int64_t _nliterals;
			rabbit::ObjectPtr *_literals;
		
			int64_t _nparameters;
			rabbit::ObjectPtr *_parameters;
		
			int64_t _nfunctions;
			rabbit::ObjectPtr *_functions;
		
			int64_t _noutervalues;
			rabbit::OuterVar *_outervalues;
		
			int64_t _ndefaultparams;
			int64_t *_defaultparams;
		
			int64_t _ninstructions;
			rabbit::Instruction _instructions[1];
	};

}
