#******************************************************************************
# Makefile                          (C) 1992-2003 Christophe de Dinechin (ddd) 
#                                                                  XL2 project 
#******************************************************************************
#
#  File Description:
#
#
#
#
#
#
#
#
#
#
#******************************************************************************
#This document is confidential.
#Do not redistribute without written permission
#******************************************************************************
#* File       : $RCSFile$
#* Revision   : $Revision$
#* Date       : $Date$
#******************************************************************************

BROOT=../

SOURCES=compiler.xl
OTHER_SOURCES=					\
	xl.scanner.xs				\
	xl.scanner.xl				\
	xl.errors.xs				\
	xl.errors.xl				\
	xl.parser.tree.xs			\
	xl.parser.tree.xl			\
	xl.parser.xs				\
	xl.parser.xl				\
	xl.translator.xs			\
	xl.translator.xl			\
	xl.textio.xl
TOCLEAN=bxl bbxl bbbxl
PRODUCT=bxl
XL=../xl

include $(BROOT)Makefile.config

test: debug bootstrap bootstrap_test bootstrap_stability_test
	echo ------ Differences: -----------------------------------------
	diff $(OBJROOT)bbxl_test/bootstrap/compiler.xl.C \
	     $(OBJROOT)bbxl_stability/bootstrap/compiler.xl.C
	echo ------ End of Differences -----------------------------------

bootstrap:
	$(MAKE) PRODUCT=bbxl XL=./bxl OBJROOT="$(OBJROOT)bbxl"
bootstrap_test:
	$(MAKE) PRODUCT=bbbxl XL=./bbxl OBJROOT="$(OBJROOT)bbxl_test"
bootstrap_stability_test:
	$(MAKE) PRODUCT=bbbbxl XL=./bbbxl OBJROOT="$(OBJROOT)bbxl_stability"


prebuild: xl_lib.h xl.syntax
xl_lib.h:
	ln -sf ../xl_lib.h .
xl.syntax:
	ln -sf ../xl.syntax .

.PRECIOUS: $(OBJDIR)/%.xl.C

$(OBJDIR)/%.xl.o: $(OBJDIR)/%.xl.C
	$V $(CXX) -I . $(CXXFLAGS) $(CPPFLAGS_$*) $(CXXFLAGS_$*) -o $@ -c $<

$(OBJDIR)/%.xl.C: %.xl $(XL) $(OTHER_SOURCES)
	$V $(XL) $< > $@
