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
	xl.constants.xl				\
	xl.semantics.xs				\
	xl.semantics.xl				\
	xl.semantics.declarations.xs		\
	xl.semantics.declarations.xl		\
	xl.semantics.functions.xs		\
	xl.semantics.functions.xl		\
	xl.semantics.overload.xs		\
	xl.semantics.overload.xl		\
	xl.semantics.instructions.xs		\
	xl.semantics.instructions.xl		\
	xl.semantics.types.xs			\
	xl.semantics.types.xl			\
	xl.semantics.types.functions.xs		\
	xl.semantics.types.functions.xl		\
	xl.semantics.types.records.xs		\
	xl.semantics.types.records.xl		\
	xl.semantics.types.generics.xs		\
	xl.semantics.writtenforms.xs		\
	xl.semantics.writtenforms.xl		\
	xl.semantics.pragmas.xl			\
	xl.codegenerator.xs			\
	xl.codegenerator.xl			\
	xl.renderer.xs				\
	xl.renderer.xl				\
	xl.codegenerator.machine.xs		\
	xl.codegenerator.machine.xl		\
	xl.bytecode.xs				\
	xl.bytecode.xl				\
	xl.optimizer.xs				\
	xl.optimizer.xl				\
	xl.textio.xl				\
	../xl_lib.h				\
	../xl.syntax				\
						\
	xl.plugin.listing.xl			\
	xl.plugin.include.xl			\
	xl.plugin.junk.xl			\
	xl.plugin.transforms.xl			\
	xl.plugin.differentiation.xl		\
	xl.plugin.constantfold.xl

TOCLEAN=nxl
PRODUCT=nxl
XL=../bootstrap/bxl

include $(BROOT)Makefile.config

qtest: debug regression

test: debug regression native native_test native_stability_test
	echo ------ Differences: -----------------------------------------
	diff $(OBJROOT)nnxl_test/native/compiler.xl.C \
	     $(OBJROOT)nnxl_stability/native/compiler.xl.C
	echo ------ End of Differences -----------------------------------

# For now, we rely on mere transcoding to get the next generation compiler
# We disable semantics, which is incomplete enough to cause more harm than good
native:
	$(MAKE) PRODUCT=nnxl XL="./nxl" XLOPTS="-bootstrap" OBJROOT="$(OBJROOT)nnxl"
native_test: 
	$(MAKE) PRODUCT=nnnxl XL="./nnxl" XLOPTS="-bootstrap" OBJROOT="$(OBJROOT)nnxl_test"
native_stability_test: 
	$(MAKE) PRODUCT=nnnnxl XL="./nnnxl" XLOPTS="-bootstrap" OBJROOT="$(OBJROOT)nnxl_stability"
regression: ALWAYS
	cd TESTS; ./alltests
ALWAYS:

prebuild: xl_lib.h xl.syntax
xl_lib.h:
	ln -sf ../xl_lib.h .
xl.syntax:
	ln -sf ../xl.syntax .

.PRECIOUS: $(OBJDIR)/%.xl.C

$(OBJDIR)/%.xl.o: $(OBJDIR)/%.xl.C
	$V $(CXX) -I . $(CXXFLAGS) $(CPPFLAGS_$*) $(CXXFLAGS_$*) -o $@ -c $<

$(OBJDIR)/%.xl.C: %.xl $(XL) $(OTHER_SOURCES)
	$V $(XL) $(XLOPTS) $< > $@
