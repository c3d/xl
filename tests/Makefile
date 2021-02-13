#******************************************************************************
# Makefile                                                          XL project
#******************************************************************************
#
#  File Description:
#
#    Testing XL source files using a makefile
#
#    This makes it possible to run tests in paralelle
#
#
#
#
#
#
#******************************************************************************
#  (C) 2021 Christophe de Dinechin <christophe@dinechin.org>
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

XL=./xl
TESTS=$(wildcard */*.xl)
RUN_TESTS=yes
TIMEOUT=30

MIQ=../make-it-quick/
include $(MIQ)rules.mk

base rebase baseline: baseline.txt
run-%:
	$(PRINT_COMMAND) $(MAKE) TESTS="$(wildcard */*$**.xl)" .tests
ref-%:
	$(PRINT_COMMAND) $(MAKE) TESTS="$(wildcard */*$**.xl)" UPDATE_REFS=yes .tests

export XL
export LD_LIBRARY_PATH=..
export DYLD_LIBRARY_PATH=..

# Check a command, and set the result accordingly
# $1 = pattern
# $2 = command
define result
( $2 && ( echo "OK" > $1.result )|| ( echo "NO" > $1.result) )
endef

# Check a command, and set the result accordingly
# $1 = pattern
# $2 = command
define command
  $(call result,$1,					\
    ( $2 2>&1 ) ;					\
    test "$$?" -eq "$(if $($1.EXIT),$($1.EXIT),0)"	\
  )
endef

TEST_OK=[ "$$(cat $1.result)" == "OK" ]

# Set the result based on whether files are identical
# $1 = target
define diff
  if $(TEST_OK); then						\
  $(UPDATE_CMD)							\
  $(call result,$1,diff $1.ref $1.output > $1.diff 2>&1)	\
  fi
endef

UPDATE_CMD=$(UPDATE_CMD_$(UPDATE_REFS))
UPDATE_CMD_yes=					\
  cp $1.output $1.ref;				\
  grep -v $1 baseline.txt > baseline-new.txt;	\
  mv baseline-new.txt baseline.txt ;

# Run a command with a timeout
# $1 = timeout
# $2 = pattern
# $3 = command
TIMEOUT_CMD:=$(shell which timeout > /dev/null 2>&1 && echo "timeout $(TIMEOUT) ")
define timeout
  (					\
    echo "TIMEOUT" > $2.result;		\
    $(call command,$2,$(TIMEOUT_CMD)$3)	\
  )
endef

XL_CMD= $(if $($1.CMD),$($1.CMD),$(XL) $($1.OPT) $1.xl)
XL_FILT=$(if $($1.FILTER),| $($1.FILTER))

# Run a simple test
# $1 = pattern
XL_TIMEOUT=$(if $($*.TIMEOUT),$($*,TIMEOUT),$(TIMEOUT))
define test
	$(call timeout,$(TIMEOUT),$*,$(XL_CMD)) $(XL_FILT) > $1.output
endef


#------------------------------------------------------------------------------
#  Rules
#------------------------------------------------------------------------------

# Compute variables for the various builds
-include $(TESTS:%=%.mk)

.tests: failures.log

GENERATE_FAILURES_LOG=						\
  ( for F in $(^:.diff=); do					\
    if [ "$$(cat $$F.result)" != "OK" ]; then			\
      if ! grep -q "$$F" baseline.txt; then			\
        echo "=== $$F FAILED ===";				\
        cat $$F.diff;						\
	(cat $$F.diff ; echo "==="; cat $$F.output) > $$F.err;  \
      fi;							\
    fi;								\
  done;								\
  for F in $(^:.diff=); do					\
    if [ "$$(cat $$F.result)" != "OK" ]; then			\
      if grep -q "$$F" baseline.txt; then			\
        echo "=== $$F FAILED (EXPECTED) ===";			\
        cat $$F.diff;						\
      fi;							\
    fi;								\
  done;								\
  for F in $(^:.diff=); do					\
    if [ "$$(cat $$F.result)" == "OK" ]; then			\
      if grep -q "$$F" baseline.txt; then			\
	echo "=== $$F IMPROVED ===";				\
      else							\
	echo "=== $$F SUCCEEDED ===";				\
      fi;							\
    fi;								\
  done ) > $@ ;							\
  $(REPORT_FAILURES)

REPORT_FAILURES=						\
 (failures=$$(grep "=== .* FAILED ===" $@ | wc -l);		\
  successes=$$(grep "=== .* SUCCEEDED ===" $@ | wc -l);		\
  expected=$$(grep "=== .* FAILED .EXPECTED. ===" $@ | wc -l);	\
  improved=$$(grep "=== .* IMPROVED ===" $@ | wc -l);		\
  if [ "$$failures" -ne "0" ]; then				\
    $(INFO) "[SUMMARY]" "=== TESTS FAILED ===" "[NO]";		\
    grep "=== .* FAILED ===" $@;				\
  elif [ "$$improved" -ne "0" ]; then				\
    $(INFO) "[SUMMARY]" "=== TESTS IMPROVED ===" "[++]";	\
    grep "=== .* IMPROVED ===" $@;				\
  else								\
    $(INFO) "[SUMMARY]" "=== TESTS PASSED ===" "[OK]";		\
  fi;								\
  $(INFO) "[SUMMARY]" "  Successful tests:  $$successes";	\
  $(INFO) "[SUMMARY]" "  Failed tests:      $$failures";	\
  $(INFO) "[SUMMARY]" "  Improved tests:    $$improved";	\
  $(INFO) "[SUMMARY]" "  Expected failures: $$expected";	\
  if [ "$$failures" -ne "0" ]; then				\
    head -20 $@ ;						\
    exit 41 ;							\
  fi)

GENERATE_BASELINE=						\
  grep "=== .* FAILED .* ===" $< |				\
  sed -e 's/=== \(.*\) FAILED\(.*\) ===/\1.xl/g' > $@


failures.log: $(TESTS:%.xl=%.diff)
	$(PRINT_GENERATE) $(GENERATE_FAILURES_LOG) $(COLORIZE)
baseline.txt: failures.log
	$(PRINT_GENERATE) $(GENERATE_BASELINE) $(COLORIZE)

%.xl.mk: %.xl ./alltests-make.awk # Makefile
	$(PRINT_GENERATE) ./alltests-make.awk $< > $@

%.xl.test: %.diff
	$(PRINT_RESULT)

%.diff: %.output %.ref
	$(PRINT_COMMAND) $(call diff,$*)

%.output: %.xl .products
	$(PRINT_COMMAND) $(call test,$*)

XL_NOREF_MSG=$(if $(UPDATE_REFS),"OK","REF?")
XL_NOREF=							\
	echo $(XL_NOREF_MSG) > $*.result	;		\
	$(INFO) "[WARNING]" "No reference for $*" "[REF?]"

%.ref:
	$(PRINT_COMMAND) $(XL_NOREF) $(COLORIZE)

xl.exe=		$(PFX.exe)xl$(EXT.exe)
xl.dll=		$(PFX.dll)xl$(EXT.dll)
recorder.dll=	$(PFX.dll)recorder$(EXT.dll)

XL_BINARIES=				\
	$(xl.exe)			\
	$(xl.dll)			\
	$(recorder.dll)

XL_FILES=				\
	xl.syntax 			\
	C.syntax			\
	builtins.xl			\
	xl.stylesheet			\
	html.stylesheet			\
	debug.stylesheet		\
	dbghtml.stylesheet

.products:	$(XL_BINARIES) $(XL_FILES)

define link

$1: $2/$1
	$$(PRINT_COPY)	ln -sf $$< $$@

endef

$(eval $(foreach b,$(XL_BINARIES),$(call link,$b,..)))
$(eval $(foreach f,$(XL_FILES),$(call link,$f,../src)))

../$(xl.dll)		: ../$(xl.exe)
../$(recorder.dll)	: ../$(xl.exe)
../$(xl.exe):
	$(PRINT_BUILD) cd .. && make

.PRECIOUS: %.diff %.output %.ref
.PHONY: %.xl.test
