#! make
#######################################################################
##
## $Id: Makefile,v 1.18 2023/02/21 09:17:28 thor Exp $
##
#######################################################################
## Makefile for the jpeg project,
## THOR Software, May 20, 2012, Thomas Richter for Accusoft 
#######################################################################
##
## 
.PHONY:		clean debug final valgrind valfinal coverage all install doc dox distrib \
		verbose profile profgen profuse Distrib.zip ISODistrib.zip view realclean \
		uninstall link linkglobal linkprofuse linkprofgen linkprof pubdistrib \
		lib libstatic libdebug tar help cleandep

all:		debug

help:
		@ echo "JPEG Linux/Solaris/AIX Edition. Available make targets:"
		@ echo "clean     : cleanup directories, remove object code"
		@ echo "            a second clean will remove autoconf generated files"
		@ echo "debug     : debug-build target without optimizations and "
		@ echo "            debugger support (default)"
		@ echo "final     : final build with optimizer enabled and no debugging"
		@ echo "valgrind  : debug build without internal memory munger, allows"
		@ echo "            to detect memory leaks with valgrind"
		@ echo "valfinal  : final build for valgrind with debug symbols"
		@ echo "coverage  : build for coverage check tools"
		@ echo "doc       : build the documentation"
		@ echo "distrib   : build distribution zip archive"
		@ echo "tar	  : build distribution tar archive"
		@ echo "verbose   : debug build with verbose logging"
		@ echo "profile   : build with profiler support"
		@ echo "profgen   : ICC build target for collecting profiling information"
		@ echo "profuse   : second stage optimizer, uses profiling information"
		@ echo "            collected with 'make profgen' generated target"
		@ echo "lib	  : generate libjpeg.so"
		@ echo "libdebug  : generate debug-able libjpeg.so"
		@ echo "libstatic : generate libjpeg.a"
		@ echo "install   : install jpeg into ~/bin/wavelet"
		@ echo "uninstall : remove jpeg from ~/bin/wavelet"
		@ echo "cleandep  : remove dependency files"

#####################################################################
## Varous Autoconf related settings                                ##
#####################################################################

configure:	configure.in
	@ rm -f automakefile autoconfig.h autoconfig.h.in
	@ autoconf
	@ autoheader

autoconfig.h.in:	configure.in
	@ autoheader

autoconfig.h:	autoconfig.h.in configure
	@ touch autoconfig.h
	@ ./configure 

##
## faked rule for the automake file to shut up
## realclean and clean targets. All others depend
## on autoconfig.h and thus on this file anyhow.
automakefile:	
	@ echo "" >automakefile

##
##
## Include autoconf settings as soon as we have them
## will be included by recursive make process
##
-include	automakefile

#####################################################################
#####################################################################
##
## Build up the absolute path for the makefile containing the
## compiler specific definitions
##
ifdef		SETTINGS
##
## Autoconf selection
COMPILER_SETTINGS=	$(shell pwd)/Makefile_Settings.$(SETTINGS)
else
##
## Fallback for realclean, clean
COMPILER_SETTINGS=	$(shell pwd)/Makefile_Settings.gcc
endif
AUTOMAKEFILE=	$(shell pwd)/automakefile

##
## Include the project specific definitions

include		Makefile.template

#####################################################################
#####################################################################

ifdef	PREFIX
INSTALLDIR	= $(PREFIX)
else
INSTALLDIR	= $(HOME)/wavelet/bin
endif

##
##

##
## sub-directory libraries, automatically computed
##

DIRLIBS		=	$(foreach dir,$(DIRS),$(dir).dir)
BUILDLIBS	=	$(foreach dir,$(DIRS),$(dir).build)
FINALLIBS	=	$(foreach dir,$(DIRS),$(dir)/lib$(dir).a)
SHAREDLIBS	=	$(foreach dir,$(DIRS),$(dir)/lib_$(dir).a)
OBJECTLIST	=	$(foreach dir,$(DIRS),$(dir)/objects.list)
LIBOBJECTLIST	=	$(foreach dir,$(DIRS),$(dir)/libobjects.list)

%.dir:
		@ $(MAKE) COMPILER_SETTINGS=$(COMPILER_SETTINGS) AUTOMAKEFILE=$(AUTOMAKEFILE) \
		--no-print-directory -C $* sub$(TARGET)

%.build:
		@ $(MAKE) COMPILER_SETTINGS=$(COMPILER_SETTINGS) AUTOMAKEFILE=$(AUTOMAKEFILE) \
		MAKEFILE=$(shell if test -f $*/Makefile.$(HWTYPE); then echo Makefile.$(HWTYPE); else echo Makefile;fi) \
		--no-print-directory -C $* -f \
		$(shell if test -f $*/Makefile.$(HWTYPE); then echo Makefile.$(HWTYPE); else echo Makefile;fi) \
		sub$(TARGET)

echo_settings:
		@ $(ECHO) "Using $(CXX) $(OPTIMIZER) $(CFLAGS) $(PTHREADCFLAGS)"

#####################################################################
#####################################################################
##
## Various link targets
##

link:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(OBJECTLIST) >objects.list
		@ $(LD) $(LDFLAGS) $(PTHREADLDFLAGS) `cat objects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(SDL_LDFLAGS) -o jpeg

linkglobal:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(OBJECTLIST) >objects.list
		@ $(LD) $(LDFLAGS) $(PTHREADLDFLAGS) $(GLOBFLAGS) `cat objects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(SDL_LDFLAGS) -o jpeg

linkprofgen:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(OBJECTLIST) >objects.list
		@ $(LD) $(LDFLAGS) $(PTHREADLDFLAGS) $(PROFGEN) `cat objects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(SDL_LDFLAGS) -o jpeg
linkprofuse:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(OBJECTLIST) >objects.list
		@ $(LD) $(LDFLAGS) $(PTHREADLDFLAGS) $(GLOBFLAGS) $(PROFUSE) `cat objects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(SDL_LDFLAGS) -o jpeg

linkprof:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(OBJECTLIST) >objects.list
		@ $(LD) $(LDFLAGS) $(PTHREADLDFLAGS) $(LDPROF) `cat objects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(SDL_LDFLAGS) -o jpeg

linkcoverage:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(OBJECTLIST) >objects.list
		@ $(LD) $(LDFLAGS) $(PTHREADLDFLAGS) $(LDCOVERAGE) `cat objects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(SDL_LDFLAGS) -o jpeg

linklib:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(LIBOBJECTLIST) >libobjects.list
		@ $(CC_ONLY) $(LDFLAGS) `cat libobjects.list | sed 's/std\/unistd.o//'` \
		  $(LDLIBS) $(PTHREADLIBS) $(LD_OPTS) -shared -o libjpeg.so

linklibstatic:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(LIBOBJECTLIST) >libobjects.list
		@ $(AR) $(AROPTS) libjpeg.a `cat libobjects.list | sed 's/std\/unistd.o//'`

linklibdebug:
		@ $(ECHO) "Linking..."
		@ $(CAT) $(LIBOBJECTLIST) >libobjects.list
		@ $(LD) $(LDFLAGS) `cat libobjects.list` \
		  $(LDLIBS) $(PTHREADLIBS) $(LD_OPTS) -shared --no-undefined -o libjpeg.so

#####################################################################
#####################################################################
##
## Various make targets
##

debug	:	autoconfig.h	
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory link

verbose	:	autoconfig.h	
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory link

final	:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory link

global	:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linkglobal

profgen	:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linkprofgen

profuse	:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linkprofuse

profile	:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linkprof

valgrind:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory link

valfinal:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory link

coverage:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linkcoverage

lib:		autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linklib

libstatic:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linklibstatic

libdebug:	autoconfig.h
	@ $(MAKE) --no-print-directory echo_settings $(BUILDLIBS) \
	TARGET="$@"
	@ $(MAKE) --no-print-directory linklibdebug

clean	:
	@ find . -name "*.d" -exec rm {} \;
	@ $(MAKE) --no-print-directory $(BUILDLIBS) \
	TARGET="$@"
	@ rm -rf *.dpi *.so jpeg gmon.out core Distrib.zip objects.list libobjects.list libjpeg.so
	@ if test -f "doc/Makefile"; then $(MAKE) --no-print-directory -C doc clean; fi
	@ rm -rf dox/html

distclean	:	realclean
	@ $(MAKE) configure
	@ $(MAKE) autoconfig.h.in

realclean	:	clean
	@ rm -f *.jpeg *.ppm *.pgm *.bmp *.pgx *.pgx_?.h *.pgx_?.raw *.jp2 *.jpc cachegrind.out* gmon.out *.zip *.tgz
	@ find . -name "*.da"   -exec rm {} \;
	@ find . -name "*.bb"   -exec rm {} \;
	@ find . -name "*.bbg"  -exec rm {} \;
	@ find . -name "*.gcov" -exec rm {} \;
	@ find . -name "*.dyn"  -exec rm {} \;
	@ find . -name "*.dpi"  -exec rm {} \;
	@ rm -rf /tmp/*.dyn /tmp/*.dpi
	@ rm -rf automakefile autoconfig.h configure

cleandep:
	@ find . -name "*.d" -exec rm {} \;

distrib	:	Distrib.zip

Distrib.zip	:	doc dox configure autoconfig.h.in
	@ touch interface/jpeg.hpp
	@ sleep 2
	@ touch configure.in
	@ sleep 2
	@ touch autoconfig.h.in
	@ sleep 2
	@ touch configure
	@ $(MAKE) --no-print-directory $(DIRLIBS) TARGET="zip"
	@ $(ZIPASCII) -r Distrib.zip README Compile.txt config.h
	@ $(ZIP) -r Distrib.zip Makefile Makefile.template Makefile_Settings.*
	@ $(ZIP) -r Distrib.zip configure configure.in automakefile.in autoconfig.h.in
	@ $(ZIP) -r Distrib.zip dox/html
	@ $(ZIP) -r Distrib.zip vs10.0 --exclude '*CVS*' 
	@ $(ZIP) -r Distrib.zip vs12.0 --exclude '*CVS*'
	@ $(ZIP) -r Distrib.zip vs15.0 --exclude '*CVS*' 

isodistrib:	ISODistrib.zip

##
## The public distribution, no accusoft-code, with the stripped headers.
ISODistrib.zip	:	doc configure autoconfig.h.in
	@ cp README.license COPYRIGHT
	@ touch interface/jpeg.hpp
	@ sleep 2
	@ touch configure.in
	@ sleep 2
	@ touch autoconfig.h.in
	@ sleep 2
	@ touch configure
	@ $(MAKE) --no-print-directory $(DIRLIBS) TARGET="isozip"
	@ $(ZIPASCII) -r ISODistrib.zip README COPYRIGHT README.license README.history Compile.txt config.h
	@ $(ZIP) -r ISODistrib.zip Makefile Makefile.template Makefile_Settings.*
	@ $(ZIP) -r ISODistrib.zip configure configure.in automakefile.in autoconfig.h.in
	@ $(ZIP) -r ISODistrib.zip vs10.0 --exclude '*CVS*'
	@ $(ZIP) -r ISODistrib.zip vs12.0 --exclude '*CVS*'
	@ $(ZIP) -r ISODistrib.zip vs15.0 --exclude '*CVS*'

gpldistrib:	GPLDistrib.zip

itudistrib:	ITUDistrib.zip

##
## The public distribution, no patented code, with the stripped headers.
GPLDistrib.zip	:	doc configure autoconfig.h.in
	@ touch interface/jpeg.hpp
	@ sleep 2
	@ touch configure.in
	@ sleep 2
	@ touch autoconfig.h.in
	@ sleep 2
	@ touch configure
	@ $(MAKE) --no-print-directory $(DIRLIBS) TARGET="gplzip"
	@ $(ZIPASCII) -r GPLDistrib.zip README README.license.gpl README.license.itu README.history Compile.txt config.h
	@ $(ZIP) -r GPLDistrib.zip Makefile Makefile.template Makefile_Settings.*
	@ $(ZIP) -r GPLDistrib.zip configure configure.in automakefile.in autoconfig.h.in
	@ $(ZIP) -r GPLDistrib.zip vs10.0 --exclude '*CVS*'
	@ $(ZIP) -r GPLDistrib.zip vs12.0 --exclude '*CVS*'
	@ $(ZIP) -r GPLDistrib.zip vs15.0 --exclude '*CVS*'

##
## The public distribution, no patented code, with the stripped headers.
ITUDistrib.zip	:	doc configure autoconfig.h.in
	@ touch interface/jpeg.hpp
	@ sleep 2
	@ touch configure.in
	@ sleep 2
	@ touch autoconfig.h.in
	@ sleep 2
	@ touch configure
	@ rm -f *~
	@ $(MAKE) --no-print-directory $(DIRLIBS) TARGET="ituzip"
	@ mv README.license README.license.back
	@ cp README.license.itu README.license
	@ $(ZIPASCII) -r ITUDistrib.zip README README.license README.history Compile.txt config.h
	@ mv README.license.back README.license
	@ $(ZIP) -r ITUDistrib.zip Makefile Makefile.template Makefile_Settings.*
	@ $(ZIP) -r ITUDistrib.zip configure configure.in automakefile.in autoconfig.h.in
	@ $(ZIP) -r ITUDistrib.zip vs10.0 --exclude '*CVS*'
	@ $(ZIP) -r ITUDistrib.zip vs12.0 --exclude '*CVS*'
	@ $(ZIP) -r ITUDistrib.zip vs15.0 --exclude '*CVS*'

tar		:	distrib.tgz

distrib.tgz	:	doc configure autoconfig.h.in
	@ touch interface/jpeg.hpp
	@ sleep 2
	@ touch configure.in
	@ sleep 2
	@ touch autoconfig.h.in
	@ sleep 2
	@ touch configure
	@ tar cf distrib.tar --exclude="CVS" --exclude=".*" --exclude="*~" --exclude="*.s" --exclude="*.d" --exclude="*.o" --exclude="*.list" --exclude="*.a" $(DIRS) 
	@ tar rf distrib.tar README README.history Compile.txt config.h
	@ tar rf distrib.tar Makefile Makefile.template Makefile_Settings.*
	@ tar rf distrib.tar configure configure.in automakefile.in autoconfig.h.in
	@ gzip distrib.tar
	@ mv distrib.tar.gz distrib.tgz

install		:
	@ mkdir -p $(INSTALLDIR)
	@ cp jpeg $(INSTALLDIR)

uninstall	:
	@ rm -rf $(INSTALLDIR)/jpeg

dox		:
	@ doxygen doxyconfig

