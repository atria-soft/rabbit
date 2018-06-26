#!/usr/bin/python
import lutin.debug as debug
import lutin.tools as tools
import os


def get_type():
	return "BINARY"

def get_desc():
	return "rabbit command line interpreter"

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
	    'cmdLine/rabbit.cpp',
	    ])
	my_module.compile_version("c++", 2011)
	my_module.add_depend([
	    'rabbit-core',
	    'rabbit-std',
	    'cxx',
	    ])
	return True


