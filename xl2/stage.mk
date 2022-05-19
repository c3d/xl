#******************************************************************************
# stage.mk                                                           XL project
#******************************************************************************
#
#  File Description:
#
#    The common rules to build a given stage of the XL compiler
#
#
#
#
#
#
#
#
#******************************************************************************
#  (C) 2022 Christophe de Dinechin <christophe@dinechin.org>
#  This software is licensed under the GNU General Public License v3
#******************************************************************************
#  This file is part of XL.
#
#  XL is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  XL is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with XL.  If not, see <https://www.gnu.org/licenses/>.
#******************************************************************************

# For each stage, we bootstrap teh compiler with previous stage,
# then build it with itself, and do a second build to check stability
VARIANTS=	boot			\
		self			\
		check

PRODUCTS=		$(VARIANT:%=xl-$(STAGE)-%.exe)
XL=			$(OUTPUT)$(XL_$(VARIANT))$(EXT.exe)
XL_boot=		xl-$(BOOTSTRAP)-check
XL_self=		xl-$(STAGE)-boot
XL_check=		xl-$(STAGE)-self

INCLUDES=.

MIQ_OBJDIR:=$(MIQ_OBJROOT)/$(MIQ_DIR)$(VARIANT:%=%/)

MIQ=../../make-it-quick/
include $(MIQ)rules.mk
$(MIQ)rules.mk:
	$(PRINT_BUILD) cd ..; git submodule update --init --recursive

MIQ_OBJDIR:=$(MIQ_OBJROOT)/$(MIQ_DIR)$(VARIANT:%=%/)

# MinGW's unable to follow symlinks, so we have to copy dependencies
.prebuild: xl_lib.h xl.syntax $(MIQ_OBJDIR).mkdir
xl_lib.h: ../$(BOOTSTRAP)/xl_lib.h
	$(PRINT_COPY) cp $< $@
xl.syntax: ../$(BOOTSTRAP)/xl.syntax
	$(PRINT_COPY) cp $< $@

# Check that the self-compiled and second check output are identical
ifeq ($(VARIANT),)
.postbuild: check-generated-code-stability
check-generated-code-stability:
	$(PRINT_TEST)	$(STAGE_OUTPUT_DIFF) && echo "[OK]" || echo "[NO]"

SELF_CODE=$(MIQ_OBJROOT)/$(MIQ_DIR)self/compiler.xl.cpp
CHECK_CODE=$(MIQ_OBJROOT)/$(MIQ_DIR)check/compiler.xl.cpp
STAGE_OUTPUT_DIFF= diff	$(SELF_CODE) $(CHECK_CODE)

else

# Generate the C++ file from XL sources
$(MIQ_OBJDIR)%.xl.cpp: %.xl $(XL) $(OTHER_SOURCES)
	$(PRINT_COMPILE) $(XL) $(XLOPTS) $< > $@

# Build the generated C++ file
$(MIQ_OBJDIR)%.xl$(EXT.obj): $(MIQ_OBJDIR)%.xl.cpp
	$(PRINT_COMPILE) $(COMPILE.cpp)

# Preserve the C++ file for inspection
.PRECIOUS: $(MIQ_OBJDIR)%.xl.cpp

# This seems necessary, but not sure why
$(MIQ_OBJROOT)/$(MIQ_DIR)compiler.xl.o:

endif

LD=$(CXX)
