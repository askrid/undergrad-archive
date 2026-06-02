VENV = .venv
SPYTHON = python3
SUBMIT = 2020-17316.zip
NAME ?= rock

run : $(VENV)
	$(VENV)/bin/python main.py $(NAME)

rock : $(VENV)
	$(VENV)/bin/python main.py rock

art : $(VENV)
	$(VENV)/bin/python main.py art

$(VENV) : requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

clean :
	rm -rf $(VENV) $(SUBMIT)

submit :
	zip -r $(SUBMIT) \
		Makefile requirements.txt README.md \
		main.py render.py shader.py control.py scene.py mesh.py

.PHONY: run rock art clean submit
