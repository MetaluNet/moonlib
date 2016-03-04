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

class.sources = absolutepath.c basedir.c char2f.c comma.c dinlet~.c dispatch.c \
dripchar.c f2char.c f2s.c gamme.c image.c mknob.c panvol~.c popen.c \
relativepath.c s2f.c sarray.c sfread2~.c slist.c ssaw~.c tabdump2.c \
tabenv.c tabreadl.c tabsort2.c tabsort.c wac.c ndmetro.c readsfv~.c

# all extra files to be included in binary distribution of the library
datafiles = \
$(wildcard *-help.pd) \
moonlib-meta.pd \
image.tcl \
LICENSE.txt \
README.txt

datadirs = img

include pd-lib-builder/Makefile.pdlibbuilder
