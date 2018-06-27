/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#pragma once

#include <rabbit/squtils.hpp>
#include <rabbit/sqobject.hpp>
#include <rabbit/RegFunction.hpp>

struct SQString;
struct SQTable;
//max number of character for a printed number
#define NUMBER_MAX_CHAR 50

struct SQStringTable
{
	SQStringTable(SQSharedState*ss);
	~SQStringTable();
	SQString *add(const rabbit::Char *,int64_t len);
	void remove(SQString *);
private:
	void resize(int64_t size);
	void allocNodes(int64_t size);
	SQString **_strings;
	uint64_t _numofslots;
	uint64_t _slotused;
	SQSharedState *_sharedstate;
};

struct RefTable {
	struct RefNode {
		rabbit::ObjectPtr obj;
		uint64_t refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void addRef(rabbit::Object &obj);
	rabbit::Bool release(rabbit::Object &obj);
	uint64_t getRefCount(rabbit::Object &obj);
	void finalize();
private:
	RefNode *get(rabbit::Object &obj,SQHash &mainpos,RefNode **prev,bool add);
	RefNode *add(SQHash mainpos,rabbit::Object &obj);
	void resize(uint64_t size);
	void allocNodes(uint64_t size);
	uint64_t _numofslots;
	uint64_t _slotused;
	RefNode *_nodes;
	RefNode *_freelist;
	RefNode **_buckets;
};

#define ADD_STRING(ss,str,len) ss->_stringtable->add(str,len)
#define REMOVE_STRING(ss,bstr) ss->_stringtable->remove(bstr)
namespace rabbit {
	class ObjectPtr;
}
struct SQSharedState
{
	SQSharedState();
	~SQSharedState();
	void init();
public:
	rabbit::Char* getScratchPad(int64_t size);
	int64_t getMetaMethodIdxByName(const rabbit::ObjectPtr &name);
	etk::Vector<rabbit::ObjectPtr> *_metamethods;
	rabbit::ObjectPtr _metamethodsmap;
	etk::Vector<rabbit::ObjectPtr> *_systemstrings;
	etk::Vector<rabbit::ObjectPtr> *_types;
	SQStringTable *_stringtable;
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
	rabbit::Char *_scratchpad;
	int64_t _scratchpadsize;
};

#define _sp(s) (_sharedstate->getScratchPad(s))
#define _spval (_sharedstate->getScratchPad(-1))

#define _table_ddel     _table(_sharedstate->_table_default_delegate)
#define _array_ddel     _table(_sharedstate->_array_default_delegate)
#define _string_ddel    _table(_sharedstate->_string_default_delegate)
#define _number_ddel    _table(_sharedstate->_number_default_delegate)
#define _generator_ddel _table(_sharedstate->_generator_default_delegate)
#define _closure_ddel   _table(_sharedstate->_closure_default_delegate)
#define _thread_ddel    _table(_sharedstate->_thread_default_delegate)
#define _class_ddel     _table(_sharedstate->_class_default_delegate)
#define _instance_ddel  _table(_sharedstate->_instance_default_delegate)
#define _weakref_ddel   _table(_sharedstate->_weakref_default_delegate)

bool compileTypemask(etk::Vector<int64_t> &res,const rabbit::Char *typemask);


