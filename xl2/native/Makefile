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
	xl.scanner.position.xs			\
	xl.scanner.position.xl			\
	xl.errors.xs				\
	xl.errors.xl				\
	xl.parser.tree.xs			\
	xl.parser.tree.xl			\
	xl.parser.xs				\
	xl.parser.xl				\
	xl.translator.xs			\
	xl.translator.xl			\
	xl.symbols.xs				\
	xl.symbols.xl				\
	xl.semantics.xs				\
	xl.semantics.xl				\
	xl.semantics.types.xs			\
	xl.semantics.types.functions.xs		\
	xl.semantics.types.generics.xs		\
	xl.semantics.declarations.xs		\
	xl.semantics.writtenforms.xs		\
	xl.codegenerator.xs			\
	xl.codegenerator.xl			\
	xl.bytecode.xs				\
	xl.bytecode.xl				\
	xl.optimizer.xs				\
	xl.optimizer.xl				\
	xl.textio.xl				\
	../xl_lib.h				\
	../xl.syntax

TOCLEAN=nxl
PRODUCT=nxl
XL=../bootstrap/bxl

include $(BROOT)Makefile.config

test: debug native native_test native_stability_test
	echo ------ Differences: -----------------------------------------
	diff $(OBJROOT)nnxl_test/native/compiler.xl.C \
	     $(OBJROOT)nnxl_stability/native/compiler.xl.C
	echo ------ End of Differences -----------------------------------

native:
	$(MAKE) PRODUCT=nnxl XL=./nxl OBJROOT="$(OBJROOT)nnxl"
native_test:
	$(MAKE) PRODUCT=nnnxl XL=./nnxl OBJROOT="$(OBJROOT)nnxl_test"
native_stability_test:
	$(MAKE) PRODUCT=nnnnxl XL=./nnnxl OBJROOT="$(OBJROOT)nnxl_stability"


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
