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
# This program is released under the GNU General Public License.
# See http://www.gnu.org/copyleft/gpl.html for details
#******************************************************************************
#* File       : $RCSFile$
#* Revision   : $Revision$
#* Date       : $Date$
#******************************************************************************

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
	xl.textio.xl				\
	../xl_lib.h				\
	../xl.syntax

VARIANTS=	cplusplus			\
		bootstrap			\
		reboot				\
		stability

PRODUCTS=		$(PRODUCTS_$(VARIANT))
PRODUCTS_cplusplus=	cxl.exe
PRODUCTS_bootstrap=	bxl.exe
PRODUCTS_reboot=	rxl.exe
PRODUCTS_stability=	sxl.exe

XL=			$(OUTPUT)$(XL_$(VARIANT))$(EXT.exe)
XL_cplusplus=		xl2c
XL_bootstrap=		cxl
XL_reboot=		bxl
XL_stability=		rxl

INCLUDES=.

MIQ=../../make-it-quick/
include $(MIQ)rules.mk
$(MIQ)rules.mk:
	$(PRINT_COMMAND) cd ../.. && git submodule update --init --recursive

ifeq ($(VARIANT),)
.tests: three-run-diff
three-run-diff:
	$(PRINT_TEST)	$(XL_DIFF)
XL_DIFF= diff								\
	$(MIQ_OBJDIR:%$(MIQ_DIR)=%/bootstrap/$(MIQ_DIR))/compiler.xl.cpp \
	$(MIQ_OBJDIR:%$(MIQ_DIR)=%/stability/$(MIQ_DIR))/compiler.xl.cpp
endif

.prebuild: xl_lib.h xl.syntax
# MinGW's unable to follow symlinks, so we have to copy dependencies
xl_lib.h: ../xl_lib.h
	$(PRINT_COPY) cp $< $@
xl.syntax: ../xl.syntax
	$(PRINT_COPY) cp $< $@

$(MIQ_OBJDIR)%.xl$(EXT.obj): $(MIQ_OBJDIR)%.xl.cpp
	$(PRINT_COMPILE) $(COMPILE.cpp)

$(MIQ_OBJDIR)%.xl.cpp: %.xl $(XL) $(OTHER_SOURCES)
	$(PRINT_COMPILE) $(XL) $< > $@
.PRECIOUS: $(MIQ_OBJDIR)%.xl.cpp

LD=$(CXX)
