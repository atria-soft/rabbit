/**
 * @author Alberto DEMICHELIS
 * @author Edouard DUPIN
 * @copyright 2018, Edouard DUPIN, all right reserved
 * @copyright 2003-2017, Alberto DEMICHELIS, all right reserved
 * @license MPL-2 (see license file)
 */


#include <stdarg.h>
#include <rabbit/VirtualMachine.hpp>


#include <rabbit/String.hpp>
#include <rabbit/Table.hpp>
#include <rabbit/Closure.hpp>
#include <rabbit/RegFunction.hpp>
#include <rabbit/NativeClosure.hpp>
#include <rabbit/FunctionProto.hpp>
#include <rabbit/Generator.hpp>
#include <rabbit/SharedState.hpp>
#include <rabbit/FunctionInfo.hpp>
#include <rabbit/StackInfos.hpp>

rabbit::Result rabbit::sq_getfunctioninfo(rabbit::VirtualMachine* v,int64_t level,rabbit::FunctionInfo *fi)
{
	int64_t cssize = v->_callsstacksize;
	if (cssize > level) {
		rabbit::VirtualMachine::callInfo &ci = v->_callsstack[cssize-level-1];
		if(ci._closure.isClosure()) {
			rabbit::Closure *c = ci._closure.toClosure();
			rabbit::FunctionProto *proto = c->_function;
			fi->funcid = proto;
			if (proto->_name.isString() == true) {
				fi->name = proto->_name.getStringValue();
			} else {
				fi->name = "unknown";
			}
			if (proto->_sourcename.isString() == true) {
				fi->source = proto->_sourcename.getStringValue();
			} else {
				fi->source = "unknown";
			}
			fi->line = proto->_lineinfos[0]._line;
			return SQ_OK;
		}
	}
	return sq_throwerror(v,"the object is not a closure");
}

rabbit::Result rabbit::sq_stackinfos(rabbit::VirtualMachine* v, int64_t level, rabbit::StackInfos *si)
{
	int64_t cssize = v->_callsstacksize;
	if (cssize > level) {
		memset(si, 0, sizeof(rabbit::StackInfos));
		rabbit::VirtualMachine::callInfo &ci = v->_callsstack[cssize-level-1];
		switch (ci._closure.getType()) {
			case rabbit::OT_CLOSURE:
				{
					rabbit::FunctionProto *func = ci._closure.toClosure()->_function;
					if (func->_name.isString() == true) {
						si->funcname = func->_name.getStringValue();
					}
					if (func->_sourcename.isString() == true) {
						si->source = func->_sourcename.getStringValue();
					}
					si->line = func->getLine(ci._ip);
				}
				break;
			case rabbit::OT_NATIVECLOSURE:
				si->source = "NATIVE";
				si->funcname = "unknown";
				if(ci._closure.toNativeClosure()->_name.isString() == true) {
					si->funcname = ci._closure.toNativeClosure()->_name.getStringValue();
				}
				si->line = -1;
				break;
			default:
				break; //shutup compiler
		}
		return SQ_OK;
	}
	return SQ_ERROR;
}

void rabbit::VirtualMachine::raise_error(const char *s, ...)
{
	va_list vl;
	va_start(vl, s);
	int64_t buffersize = (int64_t)strlen(s)+(NUMBER_UINT8_MAX*2);
	vsnprintf(_sp(sq_rsl(buffersize)),buffersize, s, vl);
	va_end(vl);
	_lasterror = rabbit::String::create(_get_shared_state(this),_spval,-1);
}

void rabbit::VirtualMachine::raise_error(const rabbit::ObjectPtr &desc)
{
	_lasterror = desc;
}

rabbit::String *rabbit::VirtualMachine::printObjVal(const rabbit::ObjectPtr &o)
{
	switch(o.getType()) {
		case rabbit::OT_STRING:
			return const_cast<rabbit::String *>(o.toString());
		case rabbit::OT_INTEGER:
			snprintf(_sp(sq_rsl(NUMBER_UINT8_MAX+1)),sq_rsl(NUMBER_UINT8_MAX), _PRINT_INT_FMT, o.toInteger());
			return rabbit::String::create(_get_shared_state(this), _spval);
			break;
		case rabbit::OT_FLOAT:
			snprintf(_sp(sq_rsl(NUMBER_UINT8_MAX+1)), sq_rsl(NUMBER_UINT8_MAX), "%.14g", o.toFloat());
			return rabbit::String::create(_get_shared_state(this), _spval);
			break;
		default:
			return rabbit::String::create(_get_shared_state(this), getTypeName(o));
	}
}

void rabbit::VirtualMachine::raise_Idxerror(const rabbit::ObjectPtr &o)
{
	rabbit::ObjectPtr oval = printObjVal(o);
	raise_error("the index '%.50s' does not exist", oval.getStringValue());
}

void rabbit::VirtualMachine::raise_Compareerror(const rabbit::Object &o1, const rabbit::Object &o2)
{
	rabbit::ObjectPtr oval1 = printObjVal(o1), oval2 = printObjVal(o2);
	raise_error("comparison between '%.50s' and '%.50s'", oval1.getStringValue(), oval2.getStringValue());
}


void rabbit::VirtualMachine::raise_ParamTypeerror(int64_t nparam,int64_t typemask,int64_t type)
{
	rabbit::ObjectPtr exptypes = rabbit::String::create(_get_shared_state(this), "", -1);
	int64_t found = 0;
	for(int64_t i=0; i<16; i++)
	{
		int64_t mask = ((int64_t)1) << i;
		if(typemask & (mask)) {
			if(found>0) stringCat(exptypes,rabbit::String::create(_get_shared_state(this), "|", -1), exptypes);
			found ++;
			stringCat(exptypes,rabbit::String::create(_get_shared_state(this), IdType2Name((rabbit::ObjectType)mask), -1), exptypes);
		}
	}
	raise_error("parameter %d has an invalid type '%s' ; expected: '%s'", nparam, IdType2Name((rabbit::ObjectType)type), exptypes.getStringValue());
}
