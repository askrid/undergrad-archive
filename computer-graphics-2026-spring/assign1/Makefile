VENV = .venv
SPYTHON = python3

run : $(VENV)
	$(VENV)/bin/python main.py

$(VENV) : requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

clean :
	rm -rf $(VENV)

.PHONY: run clean

