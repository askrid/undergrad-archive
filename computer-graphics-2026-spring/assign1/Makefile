VENV = .venv
SPYTHON = python3
SUBMIT = 2020-17316.zip

run : $(VENV)
	$(VENV)/bin/python main.py

$(VENV) : requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

clean :
	rm -rf $(VENV) $(SUBMIT)

submit :
	zip -r $(SUBMIT) \
		Makefile requirements.txt README.md \
		main.py render.py shader.py control.py scene.py \
		animate.py  primitives.py model/glados.py \
		audio/glados_wakes_up.mp3 assets/glados_screenshot.png \

.PHONY: run clean submit

