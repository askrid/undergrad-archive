# CG PA3 — Project Context

SNU 4190.410 Computer Graphics, Programming Assignment #3.
Spec: `CG_PA3.pdf` (in repo root).

## Rubric

- (a) [1pt] wireframe of rock OBJ
- (b) [3pt] Phong + Gouraud, no textures. `Ka=(0.1,0.1,0.1) Kd=(0.5,0.5,0.5) Ks=(0.8,0.8,0.8) n=4 or 8`
- (c) [2pt] textured: `Ka = AO × BaseColor`, `Kd = BaseColor`, `Ks = Specular`, `n = 1 / (a·roughness + b)`
- (d) [2pt] tangent-space normal mapping
- (e) [artistic] custom still-life scene. **No animations allowed.**

All five live in one uber shader (`shader.py`), selected by `mode` uniform 0..4.

## Layout

```
assign3/
  main.py        # entry: python main.py <model_name>  (default: rock)
  scene.py       # Material, Light, Scene, Transform, SceneItem; load_scene/save_scene/build_material
  mesh.py        # Mesh dataclass + load_obj (fan-triangulates, smooth normals if absent, Lengyel tangents)
  shader.py      # GLSL VS+FS, single program, mode uniform, 4 lights, sampler bindings cached
  render.py      # RenderWindow (orbit cam, batch draw), ShapeGroup (per-shape uniforms + textures)
  control.py     # Control: orbit/mode/save + vim hjkl transform editing
  Makefile       # run / rock / art / submit (NAME ?= rock)
  requirements.txt
  models/
    rock/        # default scene; has scene.json + Free_rock.obj + Free_rock_tex/
    art/         # artistic scene (currently empty seed scene.json)
    <anything>/  # drop a folder with scene.json + OBJs, runs via `python main.py <name>`
```

Every `models/<name>/` folder is treated identically: load `scene.json`, scan for stray OBJs and auto-add with defaults, render. No special-casing of rock vs art.

## scene.json schema

```json
{
  "lights": [{"position": [...], "color": [...], "intensity": 1.0}],
  "ambient": [r, g, b],
  "background": [r, g, b, a],
  "wire_color": [r, g, b],
  "objects": [
    {
      "name": "rock",
      "file": "Free_rock.obj",
      "mode": "wireframe|gouraud|phong|phong_tex|phong_normal",
      "transform": {"translate": [0,0,0], "rotate_deg": [0,0,0], "scale": [1,1,1]},
      "material": {
        "ka": [0.1,0.1,0.1], "kd": [0.5,0.5,0.5], "ks": [0.8,0.8,0.8],
        "shininess": 8.0, "rough_a": 200.0, "rough_b": 0.05,
        "textures": {
          "base_color": "rel/path.jpg",
          "ao": "...", "specular": "...", "roughness": "...", "normal": "..."
        }
      }
    }
  ]
}
```

All fields optional except `objects[].file`. Defaults in `scene.py` top.
Texture paths are relative to the model folder.
`rotate_deg` order: T · Ry · Rx · Rz · S (Y/X/Z intrinsic).

## Run

```bash
make            # default: rock
make art        # art folder
make NAME=foo run
python main.py <name>
```

## Controls

- Mouse: left-drag orbit, middle/right-drag pan, scroll zoom
- `R`: reset camera
- `1..5`: force shading mode (wireframe/gouraud/phong/phong_tex/phong_normal)
- `0`: clear override (use per-object mode from JSON)
- `Tab` / `Shift+Tab`: cycle selected object
- `h/l`: translate ∓X. `j/k`: translate ∓Y. `n/m`: translate ∓Z
- `Shift+h/l`: rotate ∓Y. `Shift+j/k`: rotate ∓X. `Shift+n/m`: rotate ∓Z
- `-` / `=`: uniform scale down / up
- `Enter`: save current state to `models/<name>/scene.json` (preserves textures, rewrites transforms)
- `Esc`: quit

Selected object shown in window title `CG PA3 — <name>  [i/N]  obj_name`.

## Shader (shader.py)

- VS computes `world`, `v_normal = mat3(model) * normals`, `v_tangent = mat3(model) * tangents`
- `phong()` GLSL func reused in both VS (mode==1 Gouraud) and FS
- FS branches on `mode`:
  - 0: wireframe (constant `wire_color`)
  - 1: Gouraud (use `v_gouraud` from VS)
  - 2: Phong with literal `Ka/Kd/Ks/shininess`
  - 3: Phong + textures (`kd = base`, `ka = base*ao`, `ks = specular`, `n = 1/(rough_a*rough + rough_b)`)
  - 4: same as 3 + tangent-space normal map (`TBN * (sample*2-1)`)
- Sampler uniforms bound once at program creation: `base_color_map=0 ao_map=1 specular_map=2 roughness_map=3 normal_map=4`
- 4 point lights max; attenuation `1/(1 + 0.05d + 0.01d²)` per light

## Render pipeline (render.py)

- One `pyglet.graphics.Batch` per window
- One `ShapeGroup` per object — `set_state()` writes `model`, `mode`, `Ka/Kd/Ks/shininess/rough_*`, binds 5 textures, sets polygon mode
- Global uniforms (`view_proj cam_pos ambient_color wire_color lights[]`) written once per frame in `on_draw` before `batch.draw()`
- Camera: orbit with yaw/pitch/distance/target (Vec3); `Mat4.look_at` view; `Mat4.perspective_projection` proj
- Depth test + back-face cull enabled

## Mesh loader (mesh.py)

- `load_obj(path, normalize=True)`:
  - parses `v vt vn f`, fan-triangulates n-gons
  - dedupes by `(pi, ti, ni)` tuple → indexed VBO
  - if no normals: generates smooth normals by averaging face normals
  - if `normalize=True`: recenters to AABB centre, scales so longest axis spans `[-1, 1]`
  - tangents: Lengyel's method per-face, summed per vertex, Gram-Schmidt vs normal
- `Mesh` is float-list dataclass: `positions / normals / uvs / tangents / indices`

## Known constraints / gotchas

- **No animation** allowed per spec — keep `update()` empty except camera refresh; no time-dependent uniforms.
- **No shadows** implemented. Lights pass through occluders. Would need shadow mapping (extra FBO per light + depth sampling) — out of scope.
- **No transparency** in shader. Stick to opaque props.
- pyglet on macOS: `key.ISO_LEFT_TAB` does **not** exist — use `key.TAB` with `MOD_SHIFT` check.
- pyglet `Mat4.from_scale(Vec3)` exists in 2.x — verified.
- macOS Cocoa teardown prints noise on quit (`-[NSWindow ... deallocated ...]`); ignore.
- `load_texture` uses `Image.FLIP_TOP_BOTTOM` to match OpenGL UV convention.
- `Texture2D.handle` stored as Python int (`int(tex.value)` from `GLuint`).
- Pyglet event handlers reassigned by `window.on_x = self.on_x` (not `@window.event`); the existing code path.
- macOS pyglet event stall: heavy GL in event handler kills event pump → defer with `schedule_once` (see user memory).

## User preferences for this work

- "Linus Torvalds" review style: remove dead code, simplify naming, no unnecessary abstractions.
- Default to no comments. Only explain non-obvious WHY. No "what" comments.
- Type checking and tests don't verify feature correctness — say so if can't run UI.
- For the artistic scene the user is into: anime / movies / TV / rock music / skateboarding / freeskiing / mountain biking / general nerd subculture.
- Caveman mode in chat — terse fragments OK, code/commits normal.

## Pending work

- **Artistic scene (e)**: assemble `models/<custom>/` with hand-grabbed OBJs + textures + `scene.json`. Recommended composition (from prior chat): rock-band rehearsal corner — Marshall amp (normal map showcase), guitar on stand, vinyl record, pedalboard, poster plane. 3 default lights already act as stage key/fill/rim. Free OBJ sources: Sketchfab, Free3D, Polyhaven (textures).
- **Screenshots**: spec wants stills for each rubric item (a–e). Use macOS `Cmd+Shift+4` or pyglet `pyglet.image.get_buffer_manager().get_color_buffer().save(...)` from within a debug session if needed.
- Submission: `make submit` → `2020-17316.zip` includes `Makefile requirements.txt README.md main.py render.py shader.py control.py scene.py mesh.py`. No README yet — write before submit.

## Smoke test

```bash
.venv/bin/python -c "
import pyglet
pyglet.app.run = lambda *a, **k: print('app.run stubbed')
import sys; sys.argv = ['main.py']
import main; main.main()
print('OK')
" 2>&1 | grep -v -E 'Cocoa|ApplePersistence|deallocated|NSWindow|terminating|^$'
```

Repeat with `sys.argv = ['main.py', 'art']` for art folder.
Roundtrip JSON test in prior session: `scene.load_scene` → mutate transform → `save_scene` → reload → values preserved.
