PRODUCTS=HANDBOOK.pdf HANDBOOK.html

all: $(PRODUCTS)

open: $(PRODUCTS:%=open-%)
open-%: %
	open $*
linux: $(PRODUCTS:%=linux-%)
linux-HANDBOOK.pdf: HANDBOOK.pdf
	okular HANDBOOK.pdf
linux-HANDBOOK.html: HANDBOOK.html
	firefox HANDBOOK.html

%.html: %.adoc
	asciidoctor -d book $< -o $@
%.pdf: %.adoc
	asciidoctor-pdf -d book $< -o $@
