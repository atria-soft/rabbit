/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

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
			FunctionProto *create(rabbit::SharedState *ss,int64_t ninstructions,
				int64_t nliterals,int64_t nparameters,
				int64_t nfunctions,int64_t noutervalues,
				int64_t nlineinfos,int64_t nlocalvarinfos,int64_t ndefaultparams)
			{
				rabbit::FunctionProto *f;
				//I compact the whole class and members in a single memory allocation
				f = (rabbit::FunctionProto *)sq_vm_malloc(_FUNC_SIZE(ninstructions,nliterals,nparameters,nfunctions,noutervalues,nlineinfos,nlocalvarinfos,ndefaultparams));
				new ((char*)f) rabbit::FunctionProto(ss);
				f->_ninstructions = ninstructions;
				f->_literals = (rabbit::ObjectPtr*)&f->_instructions[ninstructions];
				f->_nliterals = nliterals;
				f->_parameters = (rabbit::ObjectPtr*)&f->_literals[nliterals];
				f->_nparameters = nparameters;
				f->_functions = (rabbit::ObjectPtr*)&f->_parameters[nparameters];
				f->_nfunctions = nfunctions;
				f->_outervalues = (rabbit::OuterVar*)&f->_functions[nfunctions];
				f->_noutervalues = noutervalues;
				f->_lineinfos = (rabbit::LineInfo *)&f->_outervalues[noutervalues];
				f->_nlineinfos = nlineinfos;
				f->_localvarinfos = (rabbit::LocalVarInfo *)&f->_lineinfos[nlineinfos];
				f->_nlocalvarinfos = nlocalvarinfos;
				f->_defaultparams = (int64_t *)&f->_localvarinfos[nlocalvarinfos];
				f->_ndefaultparams = ndefaultparams;
		
				_CONSTRUCT_VECTOR(rabbit::ObjectPtr,f->_nliterals,f->_literals);
				_CONSTRUCT_VECTOR(rabbit::ObjectPtr,f->_nparameters,f->_parameters);
				_CONSTRUCT_VECTOR(rabbit::ObjectPtr,f->_nfunctions,f->_functions);
				_CONSTRUCT_VECTOR(rabbit::OuterVar,f->_noutervalues,f->_outervalues);
				//_CONSTRUCT_VECTOR(rabbit::LineInfo,f->_nlineinfos,f->_lineinfos); //not required are 2 integers
				_CONSTRUCT_VECTOR(rabbit::LocalVarInfo,f->_nlocalvarinfos,f->_localvarinfos);
				return f;
			}
			void release(){
				_DESTRUCT_VECTOR(ObjectPtr,_nliterals,_literals);
				_DESTRUCT_VECTOR(ObjectPtr,_nparameters,_parameters);
				_DESTRUCT_VECTOR(ObjectPtr,_nfunctions,_functions);
				_DESTRUCT_VECTOR(rabbit::OuterVar,_noutervalues,_outervalues);
				//_DESTRUCT_VECTOR(rabbit::LineInfo,_nlineinfos,_lineinfos); //not required are 2 integers
				_DESTRUCT_VECTOR(rabbit::LocalVarInfo,_nlocalvarinfos,_localvarinfos);
				int64_t size = _FUNC_SIZE(_ninstructions,_nliterals,_nparameters,_nfunctions,_noutervalues,_nlineinfos,_nlocalvarinfos,_ndefaultparams);
				this->~FunctionProto();
				sq_vm_free(this,size);
			}
		
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
