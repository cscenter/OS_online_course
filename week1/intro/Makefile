SRC := $(wildcard ./*.tex) 

all: lec.pdf

lec.pdf: $(SRC)
	pdflatex lec

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav
