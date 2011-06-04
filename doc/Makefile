VERSION=
LATEXTARGET=thesis$(VERSION)

RM=rm
FIND=find

LATEX=xelatex
PDFLATEX=pdflatex
BIBTEX=bibtex
DVIPS=dvips
MAKEINDEX=makeindex
DVIVIEWER=xdvi -geometry 710x950-0+10
LATEX2HTML=latex2html

FIGURES=$(wildcard *.fig)
FIGUREOBJS=$(FIGURES:.fig=.eps)
TEXFILES=$(wildcard *.tex)

# Disable standard pattern rule:
%.dvi: %.tex

# Do not delete the following targets:
.PRECIOUS: %.aux %.bbl %.eps %.ind

%.aux: %.tex $(FIGUREOBJS) $(TEXFILES)
	@$(LATEX) $*
# Look for citations. Make sure grep never returns an error code.
	@grep "^\\\\citation" *.aux > .btmp.new || true

# If the citations are not changed, don't do anything. Otherwise replace
# the .btmp file to make sure Bibtex will be run.
	@if ( diff .btmp.new .btmp >& /dev/null ); then \
		$(RM) .btmp.new; \
	else \
		mv .btmp.new .btmp; \
	fi

	@if [ -f $*.idx ]; then cp $*.idx .itmp.new; else touch .itmp.new; fi
	@if ( diff .itmp.new .itmp >& /dev/null ); then \
		$(RM) .itmp.new; \
	else \
		mv .itmp.new .itmp; \
	fi

.btmp:

%.bbl: $(BIBFILES) .btmp
# Only use BibTeX if \bibliography occurs in the document. In that case,
# run BibTeX and recompile. .btmp is touched to prevent useless making
# next time.
	@if ( grep "^\\\\bibliography{" $*.tex > /dev/null ); then \
		$(BIBTEX) $*; \
		touch .rerun; \
	fi
	@touch .btmp

.itmp:

%.ind: .itmp
	@if [ -f $*.idx ]; then \
		$(MAKEINDEX) $*; \
		touch .rerun; \
		touch .itmp; \
	fi

%.eps:%.fig
	@echo Generating figure $@...
	@fig2dev -L ps $< $@

%.dvi: %.aux %.ind %.bbl
# Make sure the dvi-file exists; if not: recompile.
	@if [ ! -f $*.dvi ]; then \
		touch .rerun; \
	fi

	@if [ -f .rerun ]; then \
		$(RM) .rerun; \
		$(LATEX) $*; \
	else \
		$(MAKE) $*.aux; \
	fi

latex:
# Below the 'true' is included to prevent unnecessarily many errors.
	@if [ -n "${LATEXTARGET}" ]; then \
		$(MAKE) ${LATEXTARGET}.dvi; \
		true; \
	else \
		if [ `ls *.tex | wc -l` = "1" ]; then \
			$(MAKE) `basename \`ls *.tex\` .tex`.dvi; \
			true; \
		else \
			$(MAKE) `echo $$PWD|tr '/' '\n'|tail -1`.dvi; \
			true; \
		fi; \
	fi

%.pdf: %.tex
	$(PDFLATEX) $<

clean :
	@$(FIND) . -name "*.aux" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.bbl" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.out" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.log" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.lof" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.blg" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.brf" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.nlo" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.toc" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.btmp" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.itmp" -exec $(RM) -f {} \;
	@$(FIND) . -name "*.texshop" -exec $(RM) -f {} \;

clean-all : clean
	@$(RM) -f *.ps *.pdf
