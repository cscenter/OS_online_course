SRC := $(wildcard ./*.tex)
SRCS := $(filter-out lecv.tex, $(SRC))
SRCV := $(filter-out lev.tex, $(SRC))
IMG := $(wildcard ./img/*.png)

all: lec.pdf lecv.pdf

lecv.pdf: $(SRCV) $(IMG)
	pdflatex lecv

lec.pdf: $(SRCS) $(IMG)
	pdflatex lec

.PHONY: clean
clean:
	rm -f *.dvi *.ps *.pdf *.aux *.toc *.log *.blg *.bbl *.out *.snm *.nav *.vrb
