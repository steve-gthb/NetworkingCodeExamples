#!/usr/bin/python

import os
import sys
import getpass
import subprocess

try:
	from fabricate import *
except ImportError, e:
	print "Couldn't find the fabricate module."
	sys.exit(1)

maxFileName = 'eurexExample'

MAXELEROSDIR = os.environ['MAXELEROSDIR']
MAXCOMPILERDIR = os.environ['MAXCOMPILERDIR']
MAXCOMPILERNETDIR = os.environ['MAXCOMPILERNETDIR']
MAXMPTDIR = os.environ['MAXMPTDIR']
MAXTCPFPDIR = os.environ['MAXTCPFPDIR']

sources = ['main.c']
target = 'eurexExample'

cflags = ['-ggdb3', '-O0', '-std=gnu99', '-Wall', '-Werror', '-I.']
cflags += ['-I%s/include' % MAXCOMPILERDIR, '-I%s/include/slic' % MAXCOMPILERDIR]
cflags += ['-I%s/include' % MAXCOMPILERNETDIR, '-I%s/include/slicnet' % MAXCOMPILERNETDIR]
cflags += ['-I%s/include' % MAXMPTDIR]

lflags = ['-L.', '-lrt', '-lpthread']
#lflags += ['-Wl,--verbose']
lflags += ['-L%s/lib' % MAXCOMPILERDIR, '-L%s/lib' % MAXCOMPILERNETDIR]
lflags += ['-L%s/lib' % MAXMPTDIR]
lflags += ['-L%s/lib' % MAXELEROSDIR]
lflags += ['-L%s/lib' % MAXTCPFPDIR]
lflags += ['-lmaxmpt_core', '-lmaxmpt_eurex_eobi', '-lmaxmpt_eurex_eti']
lflags += ['-lmaxtcp']
lflags += ['-lslicnet', '-lslic', '-lmaxeleros']

def build():
	run('sliccompile', '%s.max' % maxFileName, '%s.o' % maxFileName)
	compile()
	link()

def compile():
	for source in sources:
		run('gcc', cflags, '-c', source, '-o', source.replace('.c', '.o'))

def link():
	objects = [s.replace('.c', '.o') for s in sources]
	objects += ['%s.o' % maxFileName]
	run('gcc', '-o', target, objects, lflags)

def clean():
	autoclean()

def re():
	clean()
	build()

if __name__ == "__main__":
	main()
