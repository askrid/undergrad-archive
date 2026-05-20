STUDENTID = 2020-17316
SUBMITFILE = PRJ2_$(STUDENTID)
SPYTHON = python3.12
VENV = .venv
FILES = run.py data.csv Makefile requirements.txt \
		README.md $(SUBMITFILE).pdf  \

run: $(VENV)
	$(VENV)/bin/python run.py

$(VENV): requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install --upgrade pip
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

submit: $(FILES)
	zip $(SUBMITFILE).zip $(FILES)

clean:
	rm -f $(SUBMITFILE).zip
	rm -rf $(VENV)

.PHONY: run submit clean

