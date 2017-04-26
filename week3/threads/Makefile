SRC := $(wildcard ./*.tex)
IMG := $(wildcard ./img/*.png)

all: lec.pdf

lec.pdf: $(SRC) $(IMG)
	pdflatex lec

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
