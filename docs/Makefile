PRODUCTS=HANDBOOK.pdf HANDBOOK.html

all: $(PRODUCTS)

open: $(PRODUCTS:%=open-%)
open-%: %
	open $*

%.html: %.adoc
	asciidoctor -d book $< -o $@
%.pdf: %.adoc
	asciidoctor-pdf -d book $< -o $@
