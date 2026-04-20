STUDENTID = 2020-17316
SUBMITFILE = PRJ1-2_$(STUDENTID)
SPYTHON = python3.12
VENV = .venv
FILES = run.py grammar.lark requirements.txt \
		README.md $(SUBMITFILE).pdf  \
		Makefile test.sh \
		DB/

run: $(VENV)
	$(VENV)/bin/python run.py

$(VENV): requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install --upgrade pip
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

test: $(VENV)
	PATH=$(VENV)/bin:$$PATH bash test.sh

submit: $(FILES)
	zip $(SUBMITFILE).zip $(FILES)

clean:
	rm -f $(SUBMITFILE).zip
	rm -rf $(VENV)

.PHONY: run test submit clean

