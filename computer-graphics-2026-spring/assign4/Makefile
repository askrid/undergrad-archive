VENV    = .venv
SPYTHON = python3
PY      = $(VENV)/bin/python
SUBMIT  = 2020-17316.zip

run : $(VENV)
	$(PY) main.py --pose look --width 800 --height 800 --spp 4 --depth 4 --light-grid 4

preview : $(VENV)
	$(PY) main.py --pose look --width 400 --height 400 --spp 2 --depth 2 --light-grid 4

compare : $(VENV)
	$(PY) main.py --pose look --width 800 --height 800 --spp 4 --light-grid 4 --compare-depth
	$(PY) main.py --pose look --width 800 --height 800 --depth 4 --light-grid 4 --compare-spp

docs/README.pdf : README.md
	pandoc README.md -o docs/README.pdf --pdf-engine=typst

$(VENV) : requirements.txt
	$(SPYTHON) -m venv $(VENV)
	$(VENV)/bin/pip install --upgrade pip
	$(VENV)/bin/pip install -r requirements.txt
	touch $(VENV)

clean :
	rm -rf $(VENV) $(SUBMIT) docs/README.pdf __pycache__ model/__pycache__ *.png

submit : docs/README.pdf
	zip -r $(SUBMIT) \
		Makefile requirements.txt README.md \
		main.py raytracer.py bvh.py geometry.py camera.py \
		flatten.py material.py image.py poses.py scene.py primitives.py \
		model/__init__.py model/glados.py \
		docs/*.png docs/README.pdf

.PHONY: run compare clean submit
