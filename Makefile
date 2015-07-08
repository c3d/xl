#******************************************************************************
# Makefile<eliot>                                                  Tao project 
#******************************************************************************
#
#  File Description:
#
#     The top-level makefile for ELIOT, the
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

%:
	cd src; $(MAKE) $@
