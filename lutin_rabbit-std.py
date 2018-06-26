#!/usr/bin/python
import lutin.debug as debug
import lutin.tools as tools
import os


def get_type():
	return "LIBRARY"

def get_desc():
	return "rabbit script interpreter (std wrapping)"

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
		'rabbit-std/sqstdaux.cpp',
		'rabbit-std/sqstdstream.cpp',
		'rabbit-std/sqstdrex.cpp',
		'rabbit-std/sqstdsystem.cpp',
		'rabbit-std/sqstdio.cpp',
		'rabbit-std/sqstdblob.cpp',
		'rabbit-std/sqstdmath.cpp',
		'rabbit-std/sqstdstring.cpp',
		])
	my_module.compile_version("c++", 2011)
	my_module.add_depend([
		'rabbit-core'
		])
	my_module.add_header_file([
		'rabbit-std/sqstdstring.hpp',
		'rabbit-std/sqstdmath.hpp',
		'rabbit-std/sqstdaux.hpp',
		'rabbit-std/sqstdsystem.hpp',
		'rabbit-std/sqstdblobimpl.hpp',
		'rabbit-std/sqstdstream.hpp',
		'rabbit-std/sqstdio.hpp',
		'rabbit-std/sqstdblob.hpp',
		])
	return True


