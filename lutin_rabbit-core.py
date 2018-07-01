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
	    'rabbit/Array.cpp',
	    'rabbit/AutoDec.cpp',
	    'rabbit/Class.cpp',
	    'rabbit/ClassMember.cpp',
	    'rabbit/Closure.cpp',
	    'rabbit/Compiler.cpp',
	    'rabbit/Delegable.cpp',
	    'rabbit/ExceptionTrap.cpp',
	    'rabbit/FuncState.cpp',
	    'rabbit/FunctionInfo.cpp',
	    'rabbit/FunctionProto.cpp',
	    'rabbit/Generator.cpp',
	    'rabbit/Hash.cpp',
	    'rabbit/Instance.cpp',
	    'rabbit/Instruction.cpp',
	    'rabbit/Lexer.cpp',
	    'rabbit/LineInfo.cpp',
	    'rabbit/LocalVarInfo.cpp',
	    'rabbit/MemberHandle.cpp',
	    'rabbit/MetaMethod.cpp',
	    'rabbit/NativeClosure.cpp',
	    'rabbit/Object.cpp',
	    'rabbit/ObjectPtr.cpp',
	    'rabbit/ObjectType.cpp',
	    'rabbit/ObjectValue.cpp',
	    'rabbit/Outer.cpp',
	    'rabbit/OuterVar.cpp',
	    'rabbit/RefCounted.cpp',
	    'rabbit/RefTable.cpp',
	    'rabbit/RegFunction.cpp',
	    'rabbit/SharedState.cpp',
	    'rabbit/StackInfos.cpp',
	    'rabbit/String.cpp',
	    'rabbit/StringTable.cpp',
	    'rabbit/Table.cpp',
	    'rabbit/UserData.cpp',
	    'rabbit/VirtualMachine.cpp',
	    'rabbit/WeakRef.cpp',
	    'rabbit/sqapi.cpp',
	    'rabbit/sqbaselib.cpp',
	    'rabbit/sqdebug.cpp',
	    'rabbit/sqmem.cpp',
	    ])
	my_module.compile_version("c++", 2011)
	my_module.add_depend([
	    'z',
	    'm',
	    'c',
	    'etk-base',
	    ])
	my_module.add_header_file([
	    'rabbit/Array.hpp',
	    'rabbit/AutoDec.hpp',
	    'rabbit/Class.hpp',
	    'rabbit/ClassMember.hpp',
	    'rabbit/Closure.hpp',
	    'rabbit/Compiler.hpp',
	    'rabbit/Delegable.hpp',
	    'rabbit/ExceptionTrap.hpp',
	    'rabbit/FuncState.hpp',
	    'rabbit/FunctionInfo.hpp',
	    'rabbit/FunctionProto.hpp',
	    'rabbit/Generator.hpp',
	    'rabbit/Hash.hpp',
	    'rabbit/Instance.hpp',
	    'rabbit/Instruction.hpp',
	    'rabbit/Lexer.hpp',
	    'rabbit/LineInfo.hpp',
	    'rabbit/LocalVarInfo.hpp',
	    'rabbit/MemberHandle.hpp',
	    'rabbit/MetaMethod.hpp',
	    'rabbit/NativeClosure.hpp',
	    'rabbit/Object.hpp',
	    'rabbit/ObjectPtr.hpp',
	    'rabbit/ObjectType.hpp',
	    'rabbit/ObjectValue.hpp',
	    'rabbit/Outer.hpp',
	    'rabbit/OuterVar.hpp',
	    'rabbit/RefCounted.hpp',
	    'rabbit/RefTable.hpp',
	    'rabbit/RegFunction.hpp',
	    'rabbit/SharedState.hpp',
	    'rabbit/StackInfos.hpp',
	    'rabbit/String.hpp',
	    'rabbit/StringTable.hpp',
	    'rabbit/Table.hpp',
	    'rabbit/UserData.hpp',
	    'rabbit/VirtualMachine.hpp',
	    'rabbit/WeakRef.hpp',
	    'rabbit/rabbit.hpp',
	    'rabbit/sqconfig.hpp',
	    'rabbit/sqopcodes.hpp',
	    'rabbit/squtils.hpp',
	    ])

	return True


