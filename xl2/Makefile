#******************************************************************************
#  Makefile                        (C) 1992-2000 Christophe de Dinechin (ddd)
#                                                                 XL2 Project
#******************************************************************************
#
#  File Description:
#
#    Base makefile calling the sub-makefiles in turn
#
#    This builds various 'stages' of the native compiler
#    - stage0 is written in C++, can build stage 1
#    - stage1 is written in stage 0 XL, can build stage 1 and stage 2
#    - stage2 is written in stage 1 XL, can build stage 2 and stage 3
#    - and so on
#
#
#******************************************************************************
#This document is distributed under the GNU General Public License
#See the enclosed COPYING file or http://www.gnu.org for information
#******************************************************************************
#* File       : $RCSFile$
#* Revision   : $Revision$
#* Date       : $Date$
#******************************************************************************

STAGES=	stage0	\
	stage1	\
	stage2


MIQ=../make-it-quick/
include $(MIQ)rules.mk
$(MIR)rules.mk:
	$(PRINT_BUILD) cd ..; git submodule update --init --recursive

.build: $(STAGES)

stage%:	.ALWAYS
	$(PRINT_GENERATE) cd stage$* && $(MIQ_RECURSE)

stage2: stage1
stage1: stage0
