/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <etk/types.hpp>
#include <etk/Vector.hpp>
#include <rabbit/sqconfig.hpp>
#include <rabbit/RefTable.hpp>
#include <rabbit/rabbit.hpp>

namespace rabbit {
	class StringTable;
	class RegFunction;
	class SharedState {
		public:
			SharedState();
			~SharedState();
			void init();
		public:
			char* getScratchPad(int64_t size);
			int64_t getMetaMethodIdxByName(const rabbit::ObjectPtr &name);
			etk::Vector<rabbit::ObjectPtr> *_metamethods;
			rabbit::ObjectPtr _metamethodsmap;
			etk::Vector<rabbit::ObjectPtr> *_systemstrings;
			etk::Vector<rabbit::ObjectPtr> *_types;
			rabbit::StringTable *_stringtable;
			RefTable _refs_table;
			rabbit::ObjectPtr _registry;
			rabbit::ObjectPtr _consts;
			rabbit::ObjectPtr _constructoridx;
			rabbit::ObjectPtr _root_vm;
			rabbit::ObjectPtr _table_default_delegate;
			static const rabbit::RegFunction _table_default_delegate_funcz[];
			rabbit::ObjectPtr _array_default_delegate;
			static const rabbit::RegFunction _array_default_delegate_funcz[];
			rabbit::ObjectPtr _string_default_delegate;
			static const rabbit::RegFunction _string_default_delegate_funcz[];
			rabbit::ObjectPtr _number_default_delegate;
			static const rabbit::RegFunction _number_default_delegate_funcz[];
			rabbit::ObjectPtr _generator_default_delegate;
			static const rabbit::RegFunction _generator_default_delegate_funcz[];
			rabbit::ObjectPtr _closure_default_delegate;
			static const rabbit::RegFunction _closure_default_delegate_funcz[];
			rabbit::ObjectPtr _thread_default_delegate;
			static const rabbit::RegFunction _thread_default_delegate_funcz[];
			rabbit::ObjectPtr _class_default_delegate;
			static const rabbit::RegFunction _class_default_delegate_funcz[];
			rabbit::ObjectPtr _instance_default_delegate;
			static const rabbit::RegFunction _instance_default_delegate_funcz[];
			rabbit::ObjectPtr _weakref_default_delegate;
			static const rabbit::RegFunction _weakref_default_delegate_funcz[];
		
			SQCOMPILERERROR _compilererrorhandler;
			SQPRINTFUNCTION _printfunc;
			SQPRINTFUNCTION _errorfunc;
			bool _debuginfo;
			bool _notifyallexceptions;
			rabbit::UserPointer _foreignptr;
			SQRELEASEHOOK _releasehook;
		private:
			char *_scratchpad;
			int64_t _scratchpadsize;
	};
	
	#define _sp(s) (_sharedstate->getScratchPad(s))
	#define _spval (_sharedstate->getScratchPad(-1))
	
	#define _table_ddel     _sharedstate->_table_default_delegate.toTable()
	#define _array_ddel     _sharedstate->_array_default_delegate.toTable()
	#define _string_ddel    _sharedstate->_string_default_delegate.toTable()
	#define _number_ddel    _sharedstate->_number_default_delegate.toTable()
	#define _generator_ddel _sharedstate->_generator_default_delegate.toTable()
	#define _closure_ddel   _sharedstate->_closure_default_delegate.toTable()
	#define _thread_ddel    _sharedstate->_thread_default_delegate.toTable()
	#define _class_ddel     _sharedstate->_class_default_delegate.toTable()
	#define _instance_ddel  _sharedstate->_instance_default_delegate.toTable()
	#define _weakref_ddel   _sharedstate->_weakref_default_delegate.toTable()
	
	bool compileTypemask(etk::Vector<int64_t> &res,const char *typemask);
	
}

