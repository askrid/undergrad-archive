STUDENTID = 2020-17316
SUBMITFILE = PRJ2_$(STUDENTID)
SPYTHON = python3.12
VENV = .venv
FILES = run.py truncate.py data.csv \
		Makefile requirements.txt \
		README.md $(SUBMITFILE).pdf  \

run: $(VENV)
	$(VENV)/bin/python run.py

truncate: $(VENV)
	$(VENV)/bin/python truncate.py

$(SUBMITFILE).pdf: README.md
	pandoc README.md -o $(SUBMITFILE).pdf --pdf-engine=typst

$(VENV): requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install --upgrade pip
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

submit: $(FILES)
	zip $(SUBMITFILE).zip $(FILES)

clean:
	rm -f $(SUBMITFILE).zip $(SUBMITFILE).pdf
	rm -rf $(VENV)

.PHONY: run truncate submit clean

