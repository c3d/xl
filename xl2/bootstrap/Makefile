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
	xl.tree.xs				\
	xl.textio.xl
PRODUCT=bxl

include $(BROOT)Makefile.config


prebuild: xl_lib.h xl.syntax
xl_lib.h:
	ln -sf ../xl_lib.h .
xl.syntax:
	ln -sf ../xl.syntax .

.PRECIOUS: $(OBJDIR)/%.xl.C

$(OBJDIR)/%.xl.o: $(OBJDIR)/%.xl.C
	$V $(CXX) -I . $(CXXFLAGS) $(CPPFLAGS_$*) $(CXXFLAGS_$*) -o $@ -c $<

$(OBJDIR)/%.xl.C: %.xl
	$V ../xl $< > $@
$(OBJDIR)/compiler.xl.C: $(OTHER_SOURCES) ../xl
