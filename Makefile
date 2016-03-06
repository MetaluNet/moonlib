# Makefile to build class 'moonlib' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.
#
# use : make pdincludepath=/path/to/pure-data/src/
#
# The following command will build the external and install the distributable 
# files into a subdirectory called build/moonlib :
#
# make install pdincludepath=../pure-data/src/ objectsdir=./build

lib.name = moonlib

uname := $(shell uname -s)
ifeq (MINGW,$(findstring MINGW,$(uname)))
ldlibs = -lpthread
endif

objects = char2f comma dispatch dripchar f2char f2s gamme image mknob panvol~ \
s2f sarray sfread2~ slist ssaw~ tabdump2 tabenv tabreadl tabsort2 tabsort ndmetro

ifneq (MINGW,$(findstring MINGW,$(uname)))
objects += absolutepath basedir dinlet~ popen readsfv~ relativepath wac
endif

class.sources = $(addsuffix .c,$(objects))

# all extra files to be included in binary distribution of the library
datafiles = \
$(addsuffix -help.pd,$(objects)) \
moonlib-meta.pd \
image.tcl \
LICENSE.txt \
README.txt

datadirs = img

include pd-lib-builder/Makefile.pdlibbuilder
