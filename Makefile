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

# lcdbitmap

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

cflags = -Wno-unused -Wno-unused-parameter

PDLIBBUILDERDIR ?= .
include $(PDLIBBUILDERDIR)/Makefile.pdlibbuilder

VERSION = $(shell git describe --abbrev=4)

update-pdlibbuilder:
	curl https://raw.githubusercontent.com/pure-data/pd-lib-builder/master/Makefile.pdlibbuilder > ./Makefile.pdlibbuilder

deken-source:
	@rm -rf build_src
	@mkdir -p build_src/moonlib
	@cp $(class.sources) _soundfile.c \
		$(datafiles) Makefile.pdlibbuilder Makefile \
			build_src/moonlib
	@cp -r $(datadirs) build_src/moonlib
	cd build_src/ ; deken upload -v $(VERSION) moonlib

deken-binary:
	@rm -rf build
	@make install objectsdir=./build
	cd build/ ; deken upload -v $(VERSION) moonlib


