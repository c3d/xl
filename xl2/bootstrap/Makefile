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
	xl.textio.xl				\
	../xl_lib.h				\
	../xl.syntax

TOCLEAN=bxl$(EXE) bbxl$(EXE) bbbxl$(EXE)
PRODUCT=bxl$(EXE)
XL=../xl$(EXE)

include $(BROOT)Makefile.config

test: debug bootstrap bootstrap_test bootstrap_stability_test
	echo ------ Differences: -----------------------------------------
	diff $(OBJROOT)bbxl_test/bootstrap/compiler.xl.C \
	     $(OBJROOT)bbxl_stability/bootstrap/compiler.xl.C
	echo ------ End of Differences -----------------------------------

bootstrap:
	$(MAKE) PRODUCT=bbxl$(EXE) XL=./bxl$(EXE) OBJROOT="$(OBJROOT)bbxl"
bootstrap_test:
	$(MAKE) PRODUCT=bbbxl$(EXE) XL=./bbxl$(EXE) OBJROOT="$(OBJROOT)bbxl_test"
bootstrap_stability_test:
	$(MAKE) PRODUCT=bbbbxl$(EXE) XL=./bbbxl$(EXE) OBJROOT="$(OBJROOT)bbxl_stability"


prebuild: xl_lib.h xl.syntax
# MinGW's unable to follow symlinks, so we have to copy dependencies
xl_lib.h:
	cp ../xl_lib.h .
xl.syntax:
	cp ../xl.syntax .

.PRECIOUS: $(OBJDIR)/%.xl.C

$(OBJDIR)/%.xl.o: $(OBJDIR)/%.xl.C
	$V $(CXX) -I . $(CXXFLAGS) $(CPPFLAGS_$*) $(CXXFLAGS_$*) -o $@ -c $<

$(OBJDIR)/%.xl.C: %.xl $(XL) $(OTHER_SOURCES)
	$V $(XL) $< > $@
