#!/usr/bin/python
import lutin.debug as debug
import lutin.tools as tools
import os


def get_type():
	return "LIBRARY"

def get_desc():
	return "rabbit script interpreter"

def get_licence():
	return "MIT"

def get_compagny_type():
	return "org"

def get_compagny_name():
	return "rabbit"

def get_maintainer():
	return ["Edouard DUPIN <edupin@gmail.com>"]

def get_version():
	return [0,1]

def configure(target, my_module):
	my_module.add_src_file([
		'rabbit/Hash.cpp',
		'rabbit/RefTable.cpp',
		'rabbit/SharedState.cpp',
		'rabbit/String.cpp',
		'rabbit/StringTable.cpp',
		'rabbit/Table.cpp',
		'rabbit/sqmem.cpp',
		'rabbit/sqbaselib.cpp',
		'rabbit/sqapi.cpp',
		'rabbit/sqlexer.cpp',
		'rabbit/Class.cpp',
		'rabbit/ClassMember.cpp',
		'rabbit/Instance.cpp',
		'rabbit/AutoDec.cpp',
		'rabbit/VirtualMachine.cpp',
		'rabbit/sqobject.cpp',
		'rabbit/sqcompiler.cpp',
		'rabbit/sqdebug.cpp',
		'rabbit/sqfuncstate.cpp',
		'rabbit/RefCounted.cpp',
		'rabbit/WeakRef.cpp',
		'rabbit/Delegable.cpp',
		'rabbit/ObjectType.cpp',
		'rabbit/ObjectValue.cpp',
		'rabbit/Object.cpp',
		'rabbit/ObjectPtr.cpp',
		'rabbit/MemberHandle.cpp',
		'rabbit/StackInfos.cpp',
		'rabbit/RegFunction.cpp',
		'rabbit/FunctionInfo.cpp',
		'rabbit/MetaMethod.cpp',
		'rabbit/ExceptionTrap.cpp',
		])
	my_module.compile_version("c++", 2011)
	my_module.add_depend([
		'z',
		'm',
		'c',
		'etk-base',
		])
	my_module.add_header_file([
		'rabbit/Hash.hpp',
		'rabbit/RefTable.hpp',
		'rabbit/SharedState.hpp',
		'rabbit/String.hpp',
		'rabbit/StringTable.hpp',
		'rabbit/Table.hpp',
		'rabbit/Class.hpp',
		'rabbit/ClassMember.hpp',
		'rabbit/Instance.hpp',
		'rabbit/AutoDec.hpp',
		'rabbit/VirtualMachine.hpp',
		'rabbit/rabbit.hpp',
		'rabbit/sqobject.hpp',
		'rabbit/sqopcodes.hpp',
		'rabbit/UserData.hpp',
		'rabbit/squtils.hpp',
		'rabbit/sqpcheader.hpp',
		'rabbit/sqfuncproto.hpp',
		'rabbit/sqconfig.hpp',
		'rabbit/sqcompiler.hpp',
		'rabbit/Array.hpp',
		'rabbit/sqclosure.hpp',
		'rabbit/sqlexer.hpp',
		'rabbit/sqfuncstate.hpp',
		'rabbit/RefCounted.hpp',
		'rabbit/WeakRef.hpp',
		'rabbit/Delegable.hpp',
		'rabbit/ObjectType.hpp',
		'rabbit/ObjectValue.hpp',
		'rabbit/Object.hpp',
		'rabbit/ObjectPtr.hpp',
		'rabbit/MemberHandle.hpp',
		'rabbit/StackInfos.hpp',
		'rabbit/RegFunction.hpp',
		'rabbit/FunctionInfo.hpp',
		'rabbit/MetaMethod.hpp',
		'rabbit/ExceptionTrap.hpp',
		])

	return True


