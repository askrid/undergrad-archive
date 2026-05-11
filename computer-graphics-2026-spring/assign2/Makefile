VENV    = .venv
SPYTHON = python3
SUBMIT  = 2020-17316.zip
PY      = $(VENV)/bin/python

# Demo cages
GRID  = model/grid.obj
CROSS = model/cross_cube.obj
ICO   = model/icosahedron.obj

# Art model directory
ART   = model/art

run : bezier

bezier : $(VENV)
	$(PY) main.py bezier $(GRID)

bspline : $(VENV)
	$(PY) main.py bspline $(GRID)

cube : $(VENV)
	$(PY) main.py catmull $(CROSS)

ico : $(VENV)
	$(PY) main.py catmull $(ICO)

# ---- Art workflow ----

eve-head : $(VENV)
	$(PY) main.py catmull $(ART)/head.obj \
		--ref $(ART)/body_surface.obj $(ART)/visor_surface.obj \
		     $(ART)/arm_l_surface.obj

eve-body : $(VENV)
	$(PY) main.py catmull $(ART)/body.obj \
		--ref $(ART)/head_surface.obj $(ART)/visor_surface.obj \
		     $(ART)/arm_l_surface.obj

eve-visor : $(VENV)
	$(PY) main.py bezier $(ART)/visor.obj \
		--ref $(ART)/head_surface.obj $(ART)/body_surface.obj

eve-arm : $(VENV)
	$(PY) main.py catmull $(ART)/arm_l.obj \
		--ref $(ART)/head_surface.obj $(ART)/body_surface.obj

eve-eye : $(VENV)
	$(PY) main.py catmull $(ART)/eye_l.obj \
		--ref $(ART)/head_surface.obj $(ART)/visor_surface.obj

eve-view : $(VENV)
	$(PY) mirror_obj.py $(ART)/arm_l_surface.obj $(ART)/arm_r_surface.obj
	$(PY) mirror_obj.py $(ART)/eye_l_surface.obj $(ART)/eye_r_surface.obj
	$(PY) view_eve.py $(ART)

eve-compose : $(VENV)
	$(PY) mirror_obj.py $(ART)/arm_l_surface.obj $(ART)/arm_r_surface.obj
	$(PY) mirror_obj.py $(ART)/eye_l_surface.obj $(ART)/eye_r_surface.obj
	$(PY) compose.py $(ART)/eve.obj \
		$(ART)/head_surface.obj $(ART)/body_surface.obj \
		$(ART)/visor_surface.obj \
		$(ART)/arm_l_surface.obj $(ART)/arm_r_surface.obj \
		$(ART)/eye_l_surface.obj $(ART)/eye_r_surface.obj

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
		primitives.py obj_io.py surfaces.py \
		gen_init_eve.py compose.py mirror_obj.py view_eve.py \
		model/**/*.obj model/*.obj \
		docs/*

.PHONY: run bezier bspline cube ico \
        eve-head eve-body eve-visor eve-arm eve-eye eve-view eve-compose \
        clean submit
