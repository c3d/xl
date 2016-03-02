#******************************************************************************
# Makefile<elfe>                                                  Elfe project 
#******************************************************************************
#
#  File Description:
#
#     The top-level makefile for ELFE, the
#     Extensible Language for the Internet of Things
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
	cd tests; ./alltests

%:
	cd src; $(MAKE) $@
