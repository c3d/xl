#******************************************************************************
# Makefile<xlr>                                                   Elfe project 
#******************************************************************************
#
#  File Description:
#
#     The top-level makefile for XLR, an extensible programming language
#
#
#
#
#
#
#
#
#******************************************************************************
# (C) 2015 Christophe de Dinechin <christophe@taodyne.com>
# (C) 2015 Taodyne SAS
#******************************************************************************

all:

check:
	cd src/tests; ./alltests

%:
	cd xlr; $(MAKE) $@
