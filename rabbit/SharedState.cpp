/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */
#include <rabbit/SharedState.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/String.hpp>
#include <rabbit/RegFunction.hpp>

static rabbit::Table *createDefaultDelegate(rabbit::SharedState *ss,const rabbit::RegFunction *funcz)
{
	int64_t i=0;
	rabbit::Table *t=rabbit::Table::create(ss,0);
	while(funcz[i].name!=0){
		rabbit::NativeClosure *nc = rabbit::NativeClosure::create(ss,funcz[i].f,0);
		nc->_nparamscheck = funcz[i].nparamscheck;
		nc->_name = rabbit::String::create(ss,funcz[i].name);
		if(funcz[i].typemask && !compileTypemask(nc->_typecheck,funcz[i].typemask))
			return NULL;
		t->newSlot(rabbit::String::create(ss,funcz[i].name),nc);
		i++;
	}
	return t;
}



rabbit::SharedState::SharedState()
{
	_compilererrorhandler = NULL;
	_printfunc = NULL;
	_errorfunc = NULL;
	_debuginfo = false;
	_notifyallexceptions = false;
	_foreignptr = NULL;
	_releasehook = NULL;
}

#define newsysstring(s) {   \
	_systemstrings->pushBack(rabbit::String::create(this,s));	\
	}

#define newmetamethod(s) {  \
	_metamethods->pushBack(rabbit::String::create(this,s));  \
	_table(_metamethodsmap)->newSlot(_metamethods->back(),(int64_t)(_metamethods->size()-1)); \
	}

bool rabbit::compileTypemask(etk::Vector<int64_t> &res,const rabbit::Char *typemask)
{
	int64_t i = 0;
	int64_t mask = 0;
	while(typemask[i] != 0) {
		switch(typemask[i]) {
			case 'o': mask |= _RT_NULL; break;
			case 'i': mask |= _RT_INTEGER; break;
			case 'f': mask |= _RT_FLOAT; break;
			case 'n': mask |= (_RT_FLOAT | _RT_INTEGER); break;
			case 's': mask |= _RT_STRING; break;
			case 't': mask |= _RT_TABLE; break;
			case 'a': mask |= _RT_ARRAY; break;
			case 'u': mask |= _RT_USERDATA; break;
			case 'c': mask |= (_RT_CLOSURE | _RT_NATIVECLOSURE); break;
			case 'b': mask |= _RT_BOOL; break;
			case 'g': mask |= _RT_GENERATOR; break;
			case 'p': mask |= _RT_USERPOINTER; break;
			case 'v': mask |= _RT_THREAD; break;
			case 'x': mask |= _RT_INSTANCE; break;
			case 'y': mask |= _RT_CLASS; break;
			case 'r': mask |= _RT_WEAKREF; break;
			case '.': mask = -1; res.pushBack(mask); i++; mask = 0; continue;
			case ' ': i++; continue; //ignores spaces
			default:
				return false;
		}
		i++;
		if(typemask[i] == '|') {
			i++;
			if(typemask[i] == 0)
				return false;
			continue;
		}
		res.pushBack(mask);
		mask = 0;

	}
	return true;
}


void rabbit::SharedState::init()
{
	_scratchpad=NULL;
	_scratchpadsize=0;
	_stringtable = (rabbit::StringTable*)SQ_MALLOC(sizeof(rabbit::StringTable));
	new (_stringtable) rabbit::StringTable(this);
	sq_new(_metamethods,etk::Vector<rabbit::ObjectPtr>);
	sq_new(_systemstrings,etk::Vector<rabbit::ObjectPtr>);
	sq_new(_types,etk::Vector<rabbit::ObjectPtr>);
	_metamethodsmap = rabbit::Table::create(this,rabbit::MT_LAST-1);
	//adding type strings to avoid memory trashing
	//types names
	newsysstring(_SC("null"));
	newsysstring(_SC("table"));
	newsysstring(_SC("array"));
	newsysstring(_SC("closure"));
	newsysstring(_SC("string"));
	newsysstring(_SC("userdata"));
	newsysstring(_SC("integer"));
	newsysstring(_SC("float"));
	newsysstring(_SC("userpointer"));
	newsysstring(_SC("function"));
	newsysstring(_SC("generator"));
	newsysstring(_SC("thread"));
	newsysstring(_SC("class"));
	newsysstring(_SC("instance"));
	newsysstring(_SC("bool"));
	//meta methods
	newmetamethod(MM_ADD);
	newmetamethod(MM_SUB);
	newmetamethod(MM_MUL);
	newmetamethod(MM_DIV);
	newmetamethod(MM_UNM);
	newmetamethod(MM_MODULO);
	newmetamethod(MM_SET);
	newmetamethod(MM_GET);
	newmetamethod(MM_TYPEOF);
	newmetamethod(MM_NEXTI);
	newmetamethod(MM_CMP);
	newmetamethod(MM_CALL);
	newmetamethod(MM_CLONED);
	newmetamethod(MM_NEWSLOT);
	newmetamethod(MM_DELSLOT);
	newmetamethod(MM_TOSTRING);
	newmetamethod(MM_NEWMEMBER);
	newmetamethod(MM_INHERITED);

	_constructoridx = rabbit::String::create(this,_SC("constructor"));
	_registry = rabbit::Table::create(this,0);
	_consts = rabbit::Table::create(this,0);
	_table_default_delegate = createDefaultDelegate(this,_table_default_delegate_funcz);
	_array_default_delegate = createDefaultDelegate(this,_array_default_delegate_funcz);
	_string_default_delegate = createDefaultDelegate(this,_string_default_delegate_funcz);
	_number_default_delegate = createDefaultDelegate(this,_number_default_delegate_funcz);
	_closure_default_delegate = createDefaultDelegate(this,_closure_default_delegate_funcz);
	_generator_default_delegate = createDefaultDelegate(this,_generator_default_delegate_funcz);
	_thread_default_delegate = createDefaultDelegate(this,_thread_default_delegate_funcz);
	_class_default_delegate = createDefaultDelegate(this,_class_default_delegate_funcz);
	_instance_default_delegate = createDefaultDelegate(this,_instance_default_delegate_funcz);
	_weakref_default_delegate = createDefaultDelegate(this,_weakref_default_delegate_funcz);
}

rabbit::SharedState::~SharedState()
{
	if(_releasehook) {
		_releasehook(_foreignptr,0);
		_releasehook = NULL;
	}
	_constructoridx.Null();
	_table(_registry)->finalize();
	_table(_consts)->finalize();
	_table(_metamethodsmap)->finalize();
	_registry.Null();
	_consts.Null();
	_metamethodsmap.Null();
	while(!_systemstrings->empty()) {
		_systemstrings->back().Null();
		_systemstrings->popBack();
	}
	_thread(_root_vm)->finalize();
	_root_vm.Null();
	_table_default_delegate.Null();
	_array_default_delegate.Null();
	_string_default_delegate.Null();
	_number_default_delegate.Null();
	_closure_default_delegate.Null();
	_generator_default_delegate.Null();
	_thread_default_delegate.Null();
	_class_default_delegate.Null();
	_instance_default_delegate.Null();
	_weakref_default_delegate.Null();
	_refs_table.finalize();
	using tmpType = etk::Vector<rabbit::ObjectPtr>;
	sq_delete(_types, tmpType);
	sq_delete(_systemstrings, tmpType);
	sq_delete(_metamethods, tmpType);
	sq_delete(_stringtable, rabbit::StringTable);
	if(_scratchpad)SQ_FREE(_scratchpad,_scratchpadsize);
}


int64_t rabbit::SharedState::getMetaMethodIdxByName(const rabbit::ObjectPtr &name) {
	if(sq_type(name) != rabbit::OT_STRING) {
		return -1;
	}
	rabbit::ObjectPtr ret;
	if(_table(_metamethodsmap)->get(name,ret)) {
		return _integer(ret);
	}
	return -1;
}


rabbit::Char* rabbit::SharedState::getScratchPad(int64_t size) {
	int64_t newsize;
	if(size>0) {
		if(_scratchpadsize < size) {
			newsize = size + (size>>1);
			_scratchpad = (rabbit::Char *)SQ_REALLOC(_scratchpad,_scratchpadsize,newsize);
			_scratchpadsize = newsize;
		} else if(_scratchpadsize >= (size<<5)) {
			newsize = _scratchpadsize >> 1;
			_scratchpad = (rabbit::Char *)SQ_REALLOC(_scratchpad,_scratchpadsize,newsize);
			_scratchpadsize = newsize;
		}
	}
	return _scratchpad;
}
