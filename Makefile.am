AUTOMAKE_OPTIONS = foreign subdir-objects
SUBDIRS = . po manual
DIST_SUBDIRS = ${SUBDIRS} examples lib5

# These are needed due to a bug in Automake
pkgdatadir = @pkgdatadir@
pkglibdir = @pkglibdir@

filterdir = ${pkglibdir}
localedir = ${datadir}/locale

PERLPROG = @PERLPROG@

AM_CPPFLAGS = -I${srcdir}/gen -I${srcdir}/common \
              -I${srcdir}/interfaces/cc/ -I${srcdir}/modules/speller/default/

SUFFIXES = .info

noinst_DATA = 
pkgdata_DATA = 
EXTRA_DIST = TODO-0.61\
             win32/Makefile\
             win32/settings.h\
             win32/dirs.h\
             win32/libstub.cpp

########################################################################
#
# libaspell and friends
#

lib_LTLIBRARIES = libaspell.la

if PSPELL_COMPATIBILITY
lib_LTLIBRARIES += libpspell.la
endif

libaspell_la_SOURCES =\
  common/cache.cpp\
  common/string.cpp\
  common/getdata.cpp\
  common/itemize.cpp\
  common/file_util.cpp\
  common/string_map.cpp\
  common/string_list.cpp\
  common/config.cpp\
  common/version.cpp\
  common/posib_err.cpp\
  common/errors.cpp\
  common/error.cpp\
  common/fstream.cpp\
  common/iostream.cpp\
  common/info.cpp\
  common/can_have_error.cpp\
  common/convert.cpp\
  common/convert_filter.cpp\
  common/speller.cpp\
  common/checker.cpp\
  common/filter.cpp\
  common/objstack.cpp \
  common/strtonum.cpp\
  common/gettext_init.cpp\
  common/file_data_util.cpp\
  modules/speller/default/readonly_ws.cpp\
  modules/speller/default/suggest.cpp\
  modules/speller/default/data.cpp\
  modules/speller/default/multi_ws.cpp\
  modules/speller/default/phonetic.cpp\
  modules/speller/default/writable.cpp\
  modules/speller/default/speller_impl.cpp\
  modules/speller/default/checker_impl.cpp\
  modules/speller/default/phonet.cpp\
  modules/speller/default/typo_editdist.cpp\
  modules/speller/default/editdist.cpp\
  modules/speller/default/primes.cpp\
  modules/speller/default/lang_impl.cpp\
  modules/speller/default/leditdist.cpp\
  modules/speller/default/affix.cpp\
  lib/word_list-c.cpp\
  lib/info-c.cpp\
  lib/mutable_container-c.cpp\
  lib/error-c.cpp\
  lib/string_map-c.cpp\
  lib/new_config.cpp\
  lib/config-c.cpp\
  lib/string_enumeration-c.cpp\
  lib/can_have_error-c.cpp\
  lib/dummy.cpp\
  lib/new_filter.cpp\
  lib/new_fmode.cpp\
  lib/string_list-c.cpp\
  lib/find_speller.cpp\
  lib/speller-c.cpp\
  lib/speller-cm.cpp\
  lib/string_pair_enumeration-c.cpp\
  lib/new_checker.cpp\
  lib/checker-c.cpp\
  lib/document_checker-c.cpp\
  lib/convert-c.cpp\
  lib/language-c.cpp\
  lib/language_types-c.cpp\
  lib/language-cm.cpp\
  lib/munch_list.cpp

libaspell_la_LIBADD =  $(LTLIBINTL) $(PTHREAD_LIB)

## Libtool to so name
## C:R:A => (C-A).(A).(R)
## 16:5:0 => 16.0.5
## 16:5:1 => 15.1.5
## 18:0:2 => 16.2.0 
## 17:0:2 => 15.2.0

if INCREMENTED_SONAME
## Use C+1:R:A
libaspell_la_LDFLAGS = -version-info 19:0:3 -no-undefined
else
libaspell_la_LDFLAGS = -version-info 18:0:3 -no-undefined
endif

if PSPELL_COMPATIBILITY
libpspell_la_SOURCES = lib/dummy.cpp

libpspell_la_LIBADD = libaspell.la

if INCREMENTED_SONAME
libpspell_la_LDFLAGS = -version-info 19:0:3 -no-undefined
else
libpspell_la_LDFLAGS = -version-info 18:0:3 -no-undefined
endif

endif

EXTRA_DIST += common/*.hpp common/*.h\
              modules/speller/default/*.hpp\
              lib/*.hpp

########################################################################
#
# Aspell Program
#

AM_CPPFLAGS += -DLOCALEDIR="$(localedir)"

bin_PROGRAMS = aspell prezip-bin

aspell_SOURCES = prog/aspell.cpp prog/check_funs.cpp prog/checker_string.cpp

aspell_LDADD = libaspell.la $(CURSES_LIB)

prezip_bin_SOURCES = prog/prezip.c

EXTRA_DIST += prog/*.hpp

########################################################################
#
# Filter Modules
#

# This is for filters which are ALWAYS static.  
# The url filter is always usefull and fairly simple.
# The genconv filter is mostly defined in the main aspell libarary
#   since it is used by other filters.  The actual genconv filter
#   is just a small wrapper.

libaspell_la_SOURCES += modules/filter/url.cpp  \
                        modules/filter/genconv.cpp
static_optfiles = modules/filter/url-filter.info\
                  modules/filter/genconv-filter.info

dynamic_optfiles = 

###
### To add a new filter follow the instruction that begin with '###'
###

### Add the .info file your filter comes with
optfiles = \
  modules/filter/email-filter.info\
  modules/filter/tex-filter.info\
  modules/filter/sgml-filter.info\
  modules/filter/html-filter.info\
  modules/filter/context-filter.info\
  modules/filter/nroff-filter.info\
  modules/filter/texinfo-filter.info

### Add any extra data files your filter uses here
fltdata = \
  modules/filter/tex.conv\
  modules/filter/texinfo.conv

### Add any extra files that should be included with the distribution
### but not installed
fltextra = \
  modules/filter/mk-tex.conv.pl\
  modules/filter/mk-texinfo.conv.pl

### Add all your aspell mode files ###
fltmodes = \
  modules/filter/modes/html.amf \
  modules/filter/modes/sgml.amf \
  modules/filter/modes/tex.amf \
  modules/filter/modes/email.amf \
  modules/filter/modes/ccpp.amf \
  modules/filter/modes/none.amf \
  modules/filter/modes/perl.amf \
  modules/filter/modes/url.amf \
  modules/filter/modes/comment.amf \
  modules/filter/modes/nroff.amf\
  modules/filter/modes/texinfo.amf

if COMPILE_IN_FILTERS

static_optfiles += ${optfiles}

### Add your filter sources here,
### starting with file containing filter class definition followed by
### file containing filter member implementation.
libaspell_la_SOURCES +=\
  modules/filter/email.cpp\
  modules/filter/tex.cpp\
  modules/filter/sgml.cpp\
  modules/filter/context.cpp\
  modules/filter/nroff.cpp\
  modules/filter/texinfo.cpp

else # not COMPILE_IN_FILTERS

dynamic_optfiles += ${optfiles}
filter_ldflags = -module -avoid-version

### Add name of filter library containing your filter. Name always
### must look like lib<filtername>-filter.la see development manual
filter_LTLIBRARIES = email-filter.la tex-filter.la\
	             sgml-filter.la context-filter.la\
                     nroff-filter.la texinfo-filter.la

email_filter_la_SOURCES = modules/filter/email.cpp
email_filter_la_LIBADD = libaspell.la 
email_filter_la_LDFLAGS  = ${filter_ldflags}

tex_filter_la_SOURCES = modules/filter/tex.cpp
tex_filter_la_LIBADD = libaspell.la
tex_filter_la_LDFLAGS  = ${filter_ldflags}

sgml_filter_la_SOURCES  = modules/filter/sgml.cpp
sgml_filter_la_LIBADD = libaspell.la
sgml_filter_la_LDFLAGS  = ${filter_ldflags}

context_filter_la_SOURCES  = modules/filter/context.cpp
context_filter_la_LIBADD = libaspell.la
context_filter_la_LDFLAGS  = ${filter_ldflags}

nroff_filter_la_SOURCES  = modules/filter/nroff.cpp
nroff_filter_la_LIBADD = libaspell.la
nroff_filter_la_LDFLAGS  = ${filter_ldflags}

texinfo_filter_la_SOURCES  = modules/filter/texinfo.cpp
texinfo_filter_la_LIBADD = libaspell.la
texinfo_filter_la_LDFLAGS  = ${filter_ldflags}

### Before this line add the corresponding <yourfilterlibrary>_SOURCES and
### <yourfilterlibrary>_LIBADD lines.  The later at least has to look
### like <yourfilterlibrary>_LIBADD = ${top_builddir}/lib/libaspell.la
### in order to make your filter build properly

endif # COMPILE_IN_FILTERS

noinst_DATA += $(static_optfiles) gen/filter.pot
pkgdata_DATA += $(dynamic_optfiles) $(fltmodes) $(fltdata)

# settings.h added as a dependency so it will get recreated if
#   the COMPILE_IN_FILTERS option changes
gen/static_filters.src.cpp: ${static_optfiles} gen/mk-static-filter.pl gen/settings.h
	${PERLPROG} gen/mk-static-filter.pl ${static_optfiles}

lib/new_filter.cpp: gen/static_filters.src.cpp

gen/filter.pot: gen/mk-filter-pot.pl ${static_optfiles} ${dynamic_optfiles}
	${PERLPROG} gen/mk-filter-pot.pl

EXTRA_DIST += ${static_optfiles} ${dynamic_optfiles} \
              ${fltmodes} ${fltdata} ${fltextra}\
              gen/mk-static-filter.pl gen/mk-filter-pot.pl\
              gen/filter.pot

CLEANFILES = gen/static_filters.src.cpp

########################################################################
#
# Mk Dirs Target 
#

common/config.cpp: gen/dirs.h

gen/dirs.h: gen/mk-dirs_h.pl
	cd gen; perl mk-dirs_h.pl ${prefix} ${pkgdatadir} ${pkglibdir}  ${sysconfdir} >  dirs.h

EXTRA_DIST += gen/mk-dirs_h.pl
CLEANFILES += gen/dirs.h

########################################################################
#
# Scripts
#

bin_SCRIPTS = scripts/run-with-aspell scripts/aspell-import \
              scripts/prezip

pkgdata_SCRIPTS = scripts/spell scripts/ispell

scripts/run-with-aspell: scripts/run-with-aspell.create
	sh ${srcdir}/scripts/run-with-aspell.create ${pkgdatadir} > scripts/run-with-aspell
	chmod 755 scripts/run-with-aspell


if PSPELL_COMPATIBILITY

bin_SCRIPTS += scripts/pspell-config 
scripts/pspell-config: scripts/mkconfig
	sh ${srcdir}/scripts/mkconfig ${VERSION} ${datadir} ${pkgdatadir}
CLEANFILES += scripts/pspell-config

endif

CLEANFILES += scripts/run-with-aspell

EXTRA_DIST += scripts/mkconfig scripts/spell scripts/ispell \
              scripts/run-with-aspell.create scripts/aspell-import\
              scripts/prezip 

########################################################################
#
# auto
#

# FIXME: add dependences

noinst_DATA += auto

mksrc = \
 auto/mk-src.in auto/mk-src.pl auto/mk-doc.pl\
 auto/MkSrc/CcHelper.pm  auto/MkSrc/Methods.pm\
 auto/MkSrc/ProcImpl.pm  auto/MkSrc/Read.pm\
 auto/MkSrc/Create.pm    auto/MkSrc/ProcCc.pm\
 auto/MkSrc/ProcNativeImpl.pm  auto/MkSrc/Type.pm\
 auto/MkSrc/Info.pm            auto/MkSrc/ProcCxx.pm\
 auto/MkSrc/ProcNative.pm      auto/MkSrc/Util.pm

#auto: @MAINTAINER_MODE_TRUE@ ${mksrc}
#	cd auto; perl mk-src.pl
#	cd auto; perl auto/mk-doc.pl
#	touch auto/auto

EXTRA_DIST += auto/auto auto/mk-src.txt ${mksrc}


########################################################################
#
# interfaces
#

include_HEADERS = interfaces/cc/aspell.h

if PSPELL_COMPATIBILITY

pspell_includedir = ${includedir}/pspell

pspell_include_HEADERS = interfaces/cc/pspell.h

endif

########################################################################
#
# Misc Top level
#

pkgdata_DATA += \
  data/cp1250.cmap data/cp1250.cset \
  data/cp1251.cmap data/cp1251.cset \
  data/cp1252.cmap data/cp1252.cset \
  data/cp1253.cmap data/cp1253.cset \
  data/cp1254.cmap data/cp1254.cset \
  data/cp1255.cmap data/cp1255.cset \
  data/cp1256.cmap data/cp1256.cset \
  data/cp1257.cmap data/cp1257.cset \
  data/cp1258.cmap data/cp1258.cset \
  data/iso-8859-1.cmap data/iso-8859-1.cset   \
  data/iso-8859-2.cmap data/iso-8859-2.cset   \
  data/iso-8859-3.cmap data/iso-8859-3.cset   \
  data/iso-8859-4.cmap data/iso-8859-4.cset   \
  data/iso-8859-5.cmap data/iso-8859-5.cset   \
  data/iso-8859-6.cmap data/iso-8859-6.cset   \
  data/iso-8859-7.cmap data/iso-8859-7.cset   \
  data/iso-8859-8.cmap data/iso-8859-8.cset   \
  data/iso-8859-9.cmap data/iso-8859-9.cset   \
  data/iso-8859-10.cmap data/iso-8859-10.cset \
  data/iso-8859-11.cmap data/iso-8859-11.cset \
  data/iso-8859-13.cmap data/iso-8859-13.cset \
  data/iso-8859-14.cmap data/iso-8859-14.cset \
  data/iso-8859-15.cmap data/iso-8859-15.cset \
  data/iso-8859-16.cmap data/iso-8859-16.cset \
  data/koi8-r.cmap data/koi8-r.cset \
  data/koi8-u.cmap data/koi8-u.cset \
  data/dvorak.kbd data/split.kbd data/standard.kbd

EXTRA_DIST += config.rpath  README ${pkgdata_DATA} m4/*.m4 \
              misc/po-filter.c

README: manual/readme.texi
	makeinfo --no-validate --no-headers $< > README

maintainer-clean-local:
	@find . \( -name '*.?pp' -o -name '*.h' -o -name '*.info' \) -print |\
	    xargs grep -l "Automatically generated file." |\
	    xargs rm

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	  rm -f precat preunzip && $(LN_S) prezip precat && $(LN_S) prezip preunzip &&\
	  rm -f word-list-compress$(EXEEXT) && $(LN_S) prezip-bin$(EXEEXT) word-list-compress$(EXEEXT)

dist-hook:
	mkdir $(distdir)/maintainer
	cp autogen config-opt config-debug TODO FIXMEs README-CVS \
           $(distdir)/maintainer

ACLOCAL_AMFLAGS = -I m4

.PHONY: .manual fake-manual

.manual:
	$(MAKE) -C manual manual
manual: .manual
	$(MAKE) -C manual manual

fake-manual: 
	$(MAKE) -C manual fake-manual
