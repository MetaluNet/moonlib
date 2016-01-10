# Makefile to build class 'moonlib' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.
#
# use : make pdincludepath=/path/to/pure-data/src/
#
# The following command will build the external and install the distributable 
# files into a subdirectory called build/helloworld :
#
# make install pdincludepath=../pure-data/src/ objectsdir=./build

lib.name = moonlib

class.sources = absolutepath.c basedir.c char2f.c comma.c dinlet~.c dispatch.c dripchar.c f2char.c f2s.c gamme.c image.c mknob.c panvol~.c popen.c relativepath.c s2f.c sarray.c sfread2~.c slist.c ssaw~.c tabdump2.c tabenv.c tabreadl.c tabsort2.c tabsort.c wac.c ndmetro.c
#SOURCES_linux = readsfv~.c


# all extra files to be included in binary distribution of the library
datafiles = absolutepath-help.pd basedir-help.pd char2f-help.pd comma-help.pd dinlet~-help.pd dispatch-help.pd dripchar-help.pd f2char-help.pd f2s-help.pd gamme-help.pd image-help.pd mknob-help.pd moonlib-meta.pd panvol~-help.pd popen-help.pd readsfv~-help.pd relativepath-help.pd s2f-help.pd sarray-help.pd sfread2~-help.pd slist-help.pd ssaw~-help.pd tabdump2-help.pd tabenv-help.pd tabreadl-help.pd tabsort2-help.pd tabsort-help.pd wac-help.pd ndmetro-help.pd LICENSE.txt README.txt 

# img/d0.gif img/d10.gif img/d11.gif img/d12.gif img/d13.gif img/d14.gif img/d15.gif img/d16.gif img/d17.gif img/d18.gif img/d19.gif img/d1.gif img/d20.gif img/d21.gif img/d22.gif img/d23.gif img/d24.gif img/d25.gif img/d26.gif img/d27.gif img/d28.gif img/d29.gif img/d2.gif img/d30.gif img/d31.gif img/d32.gif img/d33.gif img/d34.gif img/d35.gif img/d36.gif img/d37.gif img/d38.gif img/d39.gif img/d3.gif img/d40.gif img/d41.gif img/d42.gif img/d43.gif img/d44.gif img/d45.gif img/d46.gif img/d47.gif img/d48.gif img/d49.gif img/d4.gif img/d50.gif img/d51.gif img/d52.gif img/d53.gif img/d54.gif img/d55.gif img/d56.gif img/d57.gif img/d58.gif img/d59.gif img/d5.gif img/d60.gif img/d61.gif img/d62.gif img/d63.gif img/d6.gif img/d7.gif img/d8.gif img/d9.gif img/pause.gif img/play.gif img/playy.gif img/rec.gif img/saww.gif img/sin.gif img/sinw.gif img/squarew.gif

datadirs = img

include pd-lib-builder/Makefile.pdlibbuilder
