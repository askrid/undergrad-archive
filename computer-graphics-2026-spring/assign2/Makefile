VENV    = .venv
SPYTHON = python3
SUBMIT  = 2020-17316.zip
PY      = $(VENV)/bin/python

# Default scene: bezier on the 4x4 grid.
GRID = model/spline/grid.obj
CROSS = model/subdiv/cross_cube.obj
ICO  = model/subdiv/icosahedron.obj

run : run-bezier

run-bezier : $(VENV)
	$(PY) main.py bezier $(GRID)

run-bspline : $(VENV)
	$(PY) main.py bspline $(GRID)

run-cc-cube : $(VENV)
	$(PY) main.py catmull $(CROSS)

run-cc-ico : $(VENV)
	$(PY) main.py catmull $(ICO)

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
		primitives.py obj_io.py surfaces.py model/* out/*

.PHONY: run run-bezier run-bspline run-cc-cube run-cc-ico clean submit
