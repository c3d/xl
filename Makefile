#******************************************************************************
# Makefile<xl>                                                      XL project
#******************************************************************************
#
#  File Description:
#
#     The top-level makefile for XL, the eXtensible Language
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

llvm:	.ALWAYS
	mkdir -p llvm/build;						\
	cd llvm/build;							\
	cmake -DCMAKE_BUILD_TYPE=Debug -DLLVM_ENABLE_RTTI=true ..;	\
	cmake --build . --target install

%:
	cd src; $(MAKE) $@
