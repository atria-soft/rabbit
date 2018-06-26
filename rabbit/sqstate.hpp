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
struct SQString;
struct SQTable;
//max number of character for a printed number
#define NUMBER_MAX_CHAR 50

struct SQStringTable
{
	SQStringTable(SQSharedState*ss);
	~SQStringTable();
	SQString *Add(const SQChar *,int64_t len);
	void Remove(SQString *);
private:
	void Resize(int64_t size);
	void AllocNodes(int64_t size);
	SQString **_strings;
	uint64_t _numofslots;
	uint64_t _slotused;
	SQSharedState *_sharedstate;
};

struct RefTable {
	struct RefNode {
		SQObjectPtr obj;
		uint64_t refs;
		struct RefNode *next;
	};
	RefTable();
	~RefTable();
	void AddRef(SQObject &obj);
	SQBool Release(SQObject &obj);
	uint64_t GetRefCount(SQObject &obj);
	void Finalize();
private:
	RefNode *Get(SQObject &obj,SQHash &mainpos,RefNode **prev,bool add);
	RefNode *Add(SQHash mainpos,SQObject &obj);
	void Resize(uint64_t size);
	void AllocNodes(uint64_t size);
	uint64_t _numofslots;
	uint64_t _slotused;
	RefNode *_nodes;
	RefNode *_freelist;
	RefNode **_buckets;
};

#define ADD_STRING(ss,str,len) ss->_stringtable->Add(str,len)
#define REMOVE_STRING(ss,bstr) ss->_stringtable->Remove(bstr)

struct SQObjectPtr;

struct SQSharedState
{
	SQSharedState();
	~SQSharedState();
	void Init();
public:
	SQChar* GetScratchPad(int64_t size);
	int64_t GetMetaMethodIdxByName(const SQObjectPtr &name);
	SQObjectPtrVec *_metamethods;
	SQObjectPtr _metamethodsmap;
	SQObjectPtrVec *_systemstrings;
	SQObjectPtrVec *_types;
	SQStringTable *_stringtable;
	RefTable _refs_table;
	SQObjectPtr _registry;
	SQObjectPtr _consts;
	SQObjectPtr _constructoridx;
	SQObjectPtr _root_vm;
	SQObjectPtr _table_default_delegate;
	static const SQRegFunction _table_default_delegate_funcz[];
	SQObjectPtr _array_default_delegate;
	static const SQRegFunction _array_default_delegate_funcz[];
	SQObjectPtr _string_default_delegate;
	static const SQRegFunction _string_default_delegate_funcz[];
	SQObjectPtr _number_default_delegate;
	static const SQRegFunction _number_default_delegate_funcz[];
	SQObjectPtr _generator_default_delegate;
	static const SQRegFunction _generator_default_delegate_funcz[];
	SQObjectPtr _closure_default_delegate;
	static const SQRegFunction _closure_default_delegate_funcz[];
	SQObjectPtr _thread_default_delegate;
	static const SQRegFunction _thread_default_delegate_funcz[];
	SQObjectPtr _class_default_delegate;
	static const SQRegFunction _class_default_delegate_funcz[];
	SQObjectPtr _instance_default_delegate;
	static const SQRegFunction _instance_default_delegate_funcz[];
	SQObjectPtr _weakref_default_delegate;
	static const SQRegFunction _weakref_default_delegate_funcz[];

	SQCOMPILERERROR _compilererrorhandler;
	SQPRINTFUNCTION _printfunc;
	SQPRINTFUNCTION _errorfunc;
	bool _debuginfo;
	bool _notifyallexceptions;
	SQUserPointer _foreignptr;
	SQRELEASEHOOK _releasehook;
private:
	SQChar *_scratchpad;
	int64_t _scratchpadsize;
};

#define _sp(s) (_sharedstate->GetScratchPad(s))
#define _spval (_sharedstate->GetScratchPad(-1))

#define _table_ddel	 _table(_sharedstate->_table_default_delegate)
#define _array_ddel	 _table(_sharedstate->_array_default_delegate)
#define _string_ddel	_table(_sharedstate->_string_default_delegate)
#define _number_ddel	_table(_sharedstate->_number_default_delegate)
#define _generator_ddel _table(_sharedstate->_generator_default_delegate)
#define _closure_ddel   _table(_sharedstate->_closure_default_delegate)
#define _thread_ddel	_table(_sharedstate->_thread_default_delegate)
#define _class_ddel	 _table(_sharedstate->_class_default_delegate)
#define _instance_ddel  _table(_sharedstate->_instance_default_delegate)
#define _weakref_ddel   _table(_sharedstate->_weakref_default_delegate)

bool CompileTypemask(SQIntVec &res,const SQChar *typemask);


