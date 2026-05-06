# CLAUDE.md — CG PA2 (Surfaces & Subdivision)

Context for resuming on another machine.

## Status (2026-05-11)

| Pts | Task | Status |
|-----|------|--------|
| 1   | Load / render / save .obj (n-gon native)       | DONE |
| 1   | Mouse pick + drag control points               | DONE |
| 2   | Bicubic Bezier surface                         | DONE |
| 2   | Uniform cubic B-Spline surface                 | DONE |
| 2   | Catmull-Clark subdivision                      | DONE |
| 2   | Supplemental: doc, exported .objs, screenshots | TODO |
| 0–2 | Artistic model + demo video                    | TODO |

All grade-required pieces work. Outstanding: build a complex artistic
model, take screenshots, write the submission doc, record <3 min video.

## Run

```bash
make run-bezier         # python main.py bezier  model/spline/grid.obj
make run-bspline        # python main.py bspline model/spline/grid.obj
make run-cc-cube        # python main.py catmull model/subdiv/cross_cube.obj
make run-cc-ico         # python main.py catmull model/subdiv/icosahedron.obj
make submit             # zip into 2020-17316.zip
```

CLI: `python main.py <mode> <obj_path> [--rows N --cols M] [--level L]`.
**One process = one scene.** No in-app model switching (was buggy + risky
— you can lose edits with a stray number-key press).

## Controls

```
LMB drag pt   edit point
LMB drag bg   orbit
RMB / MMB     pan
scroll        zoom
S / Shift+S   subdivide ±  (Catmull-Clark)
F / Shift+F   reframe / focus selected
V             toggle sample-vert dots (sky-400)
E             export .obj  → out/<base>_<name>_*.obj
ESC           quit
```

`E` for CC writes `<base>_<name>_L0_cage.obj` and `_L1..L5.obj`. For
spline scenes it writes `_cage.obj` and `_surface.obj`.

## Architecture (8 modules, ~1170 LoC)

```
main.py        argparse entry. Builds RenderWindow + SceneManager + Control.
               HUD label re-anchored to top-left on resize.
render.py      Pyglet window. add_lines / add_points / add_mesh +
               replace_mesh_vlist (keeps group alive, swaps GL list).
               Lighting + view_proj uniforms pushed inside on_draw.
shader.py      Two GLSL 330 programs:
                - flat (pos + color, gl_PointSize = 10 baked in)
                - gouraud (pos + color + normal, ambient + diffuse +
                  specular + 1/(a + b·d + c·d²) attenuation)
primitives.py  FlatGroup + LitGroup. Each owns its own shader_program.
               FlatGroup supports depth_test=False (cage points draw on top).
control.py     Arc-ball cam (theta, phi, radius around cam_target).
               Pick = nearest cage pt within PICK_RADIUS_PX=12.
               Drag = pixel-delta → world delta in view-plane.
               Uses window.push_handlers (NOT instance-attr assignment).
obj_io.py      load_obj / save_obj (n-gon native, 0-based indices).
               triangulate() fan-splits before GL upload.
surfaces.py    Vec helpers, Bernstein + cubic-BSpline basis (+ derivs).
               _tess_patch(P, steps, basis, basis_d) → single bicubic patch.
               tile_patches() walks 4×4 windows (Bezier step=3, BSpline=1).
               catmull_clark() — n-gon native, boundary rule (6P+a+b)/8,
                  interior update (F + 2R + (n-3)P) / n.
               compute_vertex_normals() — face-normal averaging.
scene.py       Scene base + BezierScene / BSplineScene / CatmullClarkScene.
               SceneManager routes Control delegate calls + E/F/V/S keys.
model/         spline/grid.obj         4x4 flat net (16 V, 9 quads)
               subdiv/cross_cube.obj   plus-cube (32 V, 30 quads)
               subdiv/icosahedron.obj  (12 V, 20 tris)
out/           Per-mode export target. .gitignored upstream.
```

## Key design decisions

1. **Single-scene per process** — picked at launch via CLI. Removed the
   1/2/3/4 in-app switcher because (a) heavy GL work inside the key-press
   handler stalled the Cocoa event pump on macOS → mouse events stopped
   firing post-swap; (b) you'd lose in-progress edits by pressing a number
   key by accident.
2. **`replace_mesh_vlist` (not in-place vlist update)** — pyglet 2.1.x
   on macOS would render *stale* triangle positions after `vlist.vertices[:]`
   in-place writes following a topology change (CC `S` then cage edit:
   cage orbited, mesh sat still). Workaround: delete + recreate the vlist
   on the same group; keeps the shader program alive, no recompile cost.
3. **Uniforms pushed inside `on_draw`, not in a clock tick** — earlier
   `clock.schedule_interval(update, 1/60)` raced with on_draw on macOS;
   draws used the previous tick's view_proj.
4. **`window.push_handlers(on_X=fn, …)` for events**, not
   `window.on_X = fn`. The instance-attribute form is fragile against
   class-level handlers on the Window subclass.
5. **n-gon native Catmull-Clark.** No pre-triangulation. Quads + tris
   both feed CC directly; one step produces all-quad output.
6. **Boundary rule** (DeRose): boundary edges = midpoint; boundary verts
   = (6P + n₁ + n₂) / 8.
7. **Bezier / BSpline normals: `cross(Sv, Su)`**, not `cross(Su, Sv)`.
   The latter pointed *into* the surface for our parametrisation
   (P[i][j] = (j, 0, i) → Su = +x, Sv = +z, cross = -y), killing diffuse
   lighting (`max(dot(N, L), 0)` clamped to 0). Surface looked dim
   (ambient-only) until this was flipped.
8. **Cage points: order = 1_000_000 + `depth_test=False`** — always-on-top,
   stay clickable even when surface covers them (CC vertex points lie
   *on* the subdivided surface and would otherwise z-fight).
9. **Camera: spherical arc-ball around `cam_target`** (Maya tumble-pivot).
   `F` re-anchors on scene bbox; `Shift+F` snaps to selected control pt.
10. **Core-profile compatible on macOS.** No `glLineWidth`/`glPointSize`
    fixed-function calls (Apple Silicon throws GL_INVALID_VALUE). Point
    size set via `gl_PointSize` in the flat vertex shader.

## Visual style

Black background. Neutral-grey surface. Cage:

```
CAGE_LINE       slate-400  (148, 163, 184)
CAGE_POINT      amber-400  (251, 191,  36)
SELECTED_POINT  rose-500   (244,  63,  94)
SURFACE_FILL    grey       (180, 180, 180)
SUBDIV_FILL     grey       (180, 180, 180)
SURFACE_SAMPLE  sky-400    ( 56, 189, 248)   # V toggle
```

HUD: Menlo 14pt, slate-200 with slight alpha. Re-anchored on every
resize via `renderer.push_handlers(on_resize=_reanchor_hud)`.

## Bugs we hit and fixed (so we don't redo)

- GL_INVALID_VALUE from `glLineWidth(2)` / `glPointSize(10)` on core
  profile. Fixed: removed; `gl_PointSize=10.0` in shader; enabled
  `GL_PROGRAM_POINT_SIZE`.
- After scene swap (now-removed), pyglet batch routed new vlists into
  stale, deleted Group domains because we'd overridden `Group.__eq__`
  on `(class, order)`. Fixed by removing those overrides → identity-based
  equality. Scene swap later removed entirely.
- Mouse events died after first scene swap on macOS. Cause: heavy GL
  inside on_key_press stalled the Cocoa event pump. Worked around with
  `pyglet.clock.schedule_once(..., 0)` to defer the swap, but the real
  fix was removing the swap feature.
- Picking flaky / orbit failed: `dragging_point` not cleared on press
  if an earlier release was missed (release outside window, focus loss).
  Fixed by resetting drag state at the top of `on_mouse_press`.
- `cross(Su, Sv)` flipped normals; diffuse term clamped to zero so
  spline surfaces looked ambient-only. Fixed → `cross(Sv, Su)`.
- After `S` (CC level change) then editing a cage point, mesh appeared
  static during orbit. Cause: `vlist.vertices[:] = …` after a topology
  rebuild leaves the GPU drawing stale positions (pyglet 2.1.x quirk
  on macOS). Fixed via `replace_mesh_vlist`.
- HUD invisible on retina / maximized window because position used the
  local `height=800` from main.py instead of `renderer.height`. Fixed
  + on_resize hook keeps it anchored.

## Outstanding work — priority order

This is the queue for the *next* session. See the discussion in chat
for the long version; in short:

### Tool safety (do FIRST before any heavy modeling)

| Pri | Feature | Sketch |
|---|---|---|
| 1 | **Undo / redo (Z / Shift+Z)** | `deque(maxlen=64)` of `list(cage_verts)` snapshots; push on `move_point`, pop on Z, push to redo deque |
| 1 | **Versioned `E`** | find next unused `<base>_<name>_vNNN_cage.obj` index, never overwrite |
| 1 | **Autosave** | every N edits OR every X seconds → `out/.autosave/<base>_<name>_cage.obj`. Atomic via `os.replace(tmp, dst)` |
| 2 | **`--resume`** | startup: if autosave newer than source, prompt / load it |
| 2 | **Shift+E "Save as"** | prompt for name in terminal |

### Authoring helpers for complex model

| Pri | Feature | Why |
|---|---|---|
| 1 | **`make_revolution.py`** | 1D Bezier curve (4 pts) + axis → 4×N cage. ~30 LoC. Required for wheels / vases / bulbs / cylinders. |
| 1 | **Symmetry mirror toggle (M key)** | edit one side, mirror across X (or chosen axis). Halves cage-edit time for symmetric models. |
| 2 | **Multi-select** (drag-rect or shift-click) | move groups of CPs together for extrude/shape |
| 2 | **`compose.py`** | CLI: list of `(obj_path, T, R, S)` → merged composite .obj. Required to assemble multi-part artistic model. |
| 2 | **CC sharp-crease tagging** | `# crease 4 7` lines in .obj; tagged edges stay sharp under subdivision. ~30 LoC. |
| 2 | **Bezier G1 seam enforcer** | when two patches share a 4-vert boundary, snap their 2nd-row CPs to keep tangent continuity. Toggle hotkey. |
| 3 | **Hide cage / surface toggles (H / Shift+H)** | screenshot composition |
| 3 | **Wireframe overlay** | visual debugging |
| 3 | **Ground plane + grid** | scale reference, looks pro |

### Artistic model — candidate ranking

**S (top-1 ambition, multi-day):**
- Akira Kaneda bike — wheels (Bezier of revolution), fairing
  (multi-patch Bezier, G1 stitched), seat / handlebars (CC), headlight
  (Bezier of revolution). Highest reward, highest risk.
- Eva-01 head / DeLorean / F1 wing — same archetype.

**A (top-3 with bounded risk, ~weekend):**
- **Pixar Luxo Jr. lamp** — *deliberately* uses all three surface
  types: BSpline-of-revolution shade + bulb, CC on stacked boxes for
  arms, Bezier-swept cord. Clean story per part.
  **← Current pick if you want bounded risk.**
- Utah Teapot (Newell) — original 32-Bezier-patch dataset, academic
  gravitas.
- Gumbo (Pixar elephant) — CC's SIGGRAPH '78 demo figure.
- Pelican / duck — CC body + Bezier beak.
- Greek amphora — single Bezier curve revolved.

**B (cute, easy):** custom Suzanne, Pikachu, treasure chest, apple.

### Math gimmicks to show off in the submission doc

- Bezier interpolates corners + boundary curve is a pure Bezier curve →
  two patches share their boundary CPs = C0; align 2nd row → G1.
- BSpline lives in convex hull of cage.
- BSpline ≡ CC limit at regular vertices. Side-by-side compare.
- Bezier surface of revolution: 1D curve → 2D axisymmetric surface.
- CC convergence: render L0..L5 side-by-side.
- CC sharp creases (if implemented): smooth blob + sharp belt.
- Möbius / trefoil via BSpline patches with twist.

## What to do first when resuming

1. `make run-bezier` — sanity check it still launches and the help text
   is readable.
2. `make run-cc-cube`, press `S` a couple times, drag a control point,
   orbit — verify the `replace_mesh_vlist` fix is still working
   (surface should rotate with the cage, not freeze).
3. Implement priority-1 safety features (undo / versioned-save /
   autosave). **Do this before any modeling work** — Akira-scale or
   Luxo-scale builds will absolutely produce a "I lost an hour of edits"
   moment otherwise.
4. Implement `make_revolution.py` + symmetry mirror.
5. Pick the model (Luxo Jr. is my recommendation).
6. Author each part as its own cage .obj; export surfaces.
7. Write `compose.py` and assemble.
8. Screenshots + 3-min demo video + submission doc.

## References

- Bernstein basis + bicubic Bezier — standard formulation.
- Uniform cubic B-Spline:
  `(1/6) · [[1,4,1,0],[-3,0,3,0],[3,-6,3,0],[-1,3,-3,1]]`
  Implemented as a direct polynomial in `_bspline` / `_bspline_d`.
- Catmull & Clark (1978). Vertex update `(F + 2R + (n-3)P) / n` with
  F = avg adjacent face points, R = avg adjacent edge midpoints,
  n = valence.
- DeRose et al. boundary rule (piecewise cubic curve restriction along
  the boundary).
- Newell's original Utah teapot dataset (32 Bezier patches, 306 CPs).
