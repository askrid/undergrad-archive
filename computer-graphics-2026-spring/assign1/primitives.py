from __future__ import annotations

import math
from typing import Any

import pyglet
from pyglet.math import Mat4
from pyglet.gl import GL_TRIANGLES, GL_LINES

# Type aliases used across this module.
Point3 = tuple[float, float, float]
Color = tuple[int, int, int, int]


class CustomGroup(pyglet.graphics.Group):
    """
    To draw multiple 3D shapes in Pyglet, you should make a group for an object.
    """

    transform_mat: Mat4
    indexed_vertices_list: (
        Any  # pyglet.graphics.vertexdomain.IndexedVertexList (private)
    )
    shader_program: Any  # pyglet.graphics.shader.ShaderProgram (private)

    def __init__(self, transform_mat: Mat4, order: int, shader_program: Any) -> None:
        super().__init__(order)
        # Shader program is owned by RenderWindow (one shared lit + one shared
        # unlit) and passed in here so many CustomGroups share a single
        # ShaderProgram instance. No per-group compilation.
        self.shader_program = shader_program
        self.transform_mat = transform_mat
        self.indexed_vertices_list = None
        self.shader_program.use()

    def set_state(self) -> None:
        self.shader_program.use()
        self.shader_program["model"] = self.transform_mat

    def unset_state(self) -> None:
        self.shader_program.stop()

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, CustomGroup):
            return NotImplemented
        return self.order == other.order and self.parent == other.parent

    def __hash__(self) -> int:
        return hash((self.order))


class Primitive:
    """
    Base class for geometry primitives consumed by RenderWindow.add_shape.

    Every primitive exposes the same four flat containers:
      - vertices : list[float]          xyz coordinates, groups of 3
      - normals  : list[float]          xyz normals, groups of 3, parallel to vertices
      - indices  : list[int]            index buffer into vertices
      - colors   : tuple[int, ...]      RGBA bytes, groups of 4

    The ``mode`` class attribute selects the GL primitive type (triangles by
    default; Axes overrides it to GL_LINES). The ``lit`` attribute selects
    which shader program RenderWindow binds -- True routes to the Gouraud lit
    shader (uses the normals), False routes to the unlit passthrough shader
    (ignores normals; used by Axes and debug gizmos).
    """

    mode: int = GL_TRIANGLES
    lit: bool = True
    vertices: list[float]
    normals: list[float]
    indices: list[int]
    colors: tuple[int, ...]


# ---------------------------------------------------------------------------
# Helpers: triangle emission with per-vertex normals.
#
# `_emit_tri2` emits a triangle DOUBLE-SIDED -- front copy CCW with the given
# normals, back copy with reversed winding and negated normals. Double-sided
# emission keeps swept/hollow primitives visible from both sides even with
# backface culling, and negating the normals on the back copy ensures Gouraud
# lighting is correct on the back-facing side (otherwise the back would light
# as if it were still the front and look inside-out).
#
# `_face_normal` computes a flat face normal from the triangle's own corners
# via cross product. Used by the `_flat` convenience wrapper for cap faces.
# ---------------------------------------------------------------------------


def _face_normal(a: Point3, b: Point3, c: Point3) -> Point3:
    ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
    nx = uy * vz - uz * vy
    ny = uz * vx - ux * vz
    nz = ux * vy - uy * vx
    L = math.sqrt(nx * nx + ny * ny + nz * nz)
    if L < 1e-12:
        return (0.0, 0.0, 0.0)
    return (nx / L, ny / L, nz / L)


def _emit_tri2(
    verts: list[float],
    norms: list[float],
    cols: tuple[int, ...],
    a: Point3,
    b: Point3,
    c: Point3,
    na: Point3,
    nb: Point3,
    nc: Point3,
    ca: Color,
    cb: Color,
    cc: Color,
) -> tuple[int, ...]:
    """Emit triangle (a,b,c) double-sided with per-vertex normals.

    Back copy has reversed winding (a,c,b) AND negated normals so Gouraud
    lighting is correct when the camera sees the back face."""
    verts.extend(
        (
            a[0],
            a[1],
            a[2],
            b[0],
            b[1],
            b[2],
            c[0],
            c[1],
            c[2],
            a[0],
            a[1],
            a[2],
            c[0],
            c[1],
            c[2],
            b[0],
            b[1],
            b[2],
        )
    )
    norms.extend(
        (
            na[0],
            na[1],
            na[2],
            nb[0],
            nb[1],
            nb[2],
            nc[0],
            nc[1],
            nc[2],
            -na[0],
            -na[1],
            -na[2],
            -nc[0],
            -nc[1],
            -nc[2],
            -nb[0],
            -nb[1],
            -nb[2],
        )
    )
    return cols + ca + cb + cc + ca + cc + cb


def _emit_tri2_flat(
    verts: list[float],
    norms: list[float],
    cols: tuple[int, ...],
    a: Point3,
    b: Point3,
    c: Point3,
    col: Color,
) -> tuple[int, ...]:
    """Double-sided triangle with a flat face normal derived from the verts."""
    n = _face_normal(a, b, c)
    return _emit_tri2(verts, norms, cols, a, b, c, n, n, n, col, col, col)


def _emit_tri2_smooth(
    verts: list[float],
    norms: list[float],
    cols: tuple[int, ...],
    a: Point3,
    b: Point3,
    c: Point3,
    na: Point3,
    nb: Point3,
    nc: Point3,
    col: Color,
) -> tuple[int, ...]:
    """Double-sided triangle with per-vertex (analytical) normals."""
    return _emit_tri2(verts, norms, cols, a, b, c, na, nb, nc, col, col, col)


class Cube(Primitive):
    """
    Axis-aligned box primitive, centered at the origin.

      size  : extents of the box, given as
                - a single number       -> uniform cube (sx=sy=sz=size), or
                - a sequence (sx,sy,sz) -> independent extent per axis.
              Values are full extents (not half-extents), so size=1.0 fits in
              [-0.5, 0.5]^3 and size=(2,4,1) fits in [-1,1] x [-2,2] x [-0.5,0.5].
      color : single (r, g, b, a) byte tuple applied uniformly.

    Emitted as 24 unique vertices (6 faces x 4 corners) so each face can carry
    its own constant face normal -- a single shared corner cannot carry three
    different face normals, which is why the 8-vertex layout won't work for
    lit rendering.
    """

    def __init__(
        self, size: float | Point3 = 1.0, color: Color = (200, 200, 200, 255)
    ) -> None:
        if isinstance(size, (int, float)):
            sx = sy = sz = float(size)
        else:
            sx, sy, sz = float(size[0]), float(size[1]), float(size[2])
        hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
        C = color

        # Each face: (normal, [4 corners in CCW order from outside]).
        faces: list[tuple[Point3, list[Point3]]] = [
            (
                (1.0, 0.0, 0.0),
                [(hx, -hy, -hz), (hx, hy, -hz), (hx, hy, hz), (hx, -hy, hz)],
            ),
            (
                (-1.0, 0.0, 0.0),
                [(-hx, -hy, -hz), (-hx, -hy, hz), (-hx, hy, hz), (-hx, hy, -hz)],
            ),
            (
                (0.0, 1.0, 0.0),
                [(hx, hy, hz), (hx, hy, -hz), (-hx, hy, -hz), (-hx, hy, hz)],
            ),
            (
                (0.0, -1.0, 0.0),
                [(hx, -hy, hz), (-hx, -hy, hz), (-hx, -hy, -hz), (hx, -hy, -hz)],
            ),
            (
                (0.0, 0.0, 1.0),
                [(-hx, -hy, hz), (hx, -hy, hz), (hx, hy, hz), (-hx, hy, hz)],
            ),
            (
                (0.0, 0.0, -1.0),
                [(hx, -hy, -hz), (-hx, -hy, -hz), (-hx, hy, -hz), (hx, hy, -hz)],
            ),
        ]

        self.vertices = []
        self.normals = []
        self.indices = []
        for n, quad in faces:
            base = len(self.vertices) // 3
            for p in quad:
                self.vertices.extend(p)
                self.normals.extend(n)
            self.indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])
        self.colors = C * 24


class Ellipsoid(Primitive):
    """
    Ellipsoid with optional angular cuts. Angles in DEGREES.
      radius       : (rx, ry, rz) semi-axes
      theta_range : longitudinal sweep in degrees; default (0, 360) is closed
      phi_range   : latitudinal sweep in degrees; default (-90, 90) is full
      color       : single (r, g, b, a) byte tuple applied uniformly
      caps        : if True, fill partial cuts with flat caps

    Surface normals are the analytical gradient normal
      N(phi, theta) = normalize( (x/rx^2, y/ry^2, z/rz^2) ).
    For a sphere this collapses to the unit position vector P/r.
    """

    def __init__(
        self,
        stacks: int = 20,
        slices: int = 30,
        radius: Point3 = (1.0, 1.0, 1.0),
        theta_range: tuple[float, float] = (0, 360),
        phi_range: tuple[float, float] = (-90, 90),
        color: Color = (180, 120, 120, 255),
        caps: bool = True,
    ) -> None:
        rx, ry, rz = radius[0], radius[1], radius[2]
        t0 = math.radians(theta_range[0])
        t1 = math.radians(theta_range[1])
        p0 = math.radians(phi_range[0])
        p1 = math.radians(phi_range[1])
        C = color

        self.vertices = []
        self.normals = []
        self.indices = []
        self.colors = ()

        def P(phi: float, theta: float) -> Point3:
            cp = math.cos(phi)
            return (
                rx * cp * math.cos(theta),
                ry * math.sin(phi),
                rz * cp * math.sin(-theta),
            )

        def N(phi: float, theta: float) -> Point3:
            cp = math.cos(phi)
            nx = cp * math.cos(theta) / rx
            ny = math.sin(phi) / ry
            nz = -cp * math.sin(theta) / rz
            L = math.sqrt(nx * nx + ny * ny + nz * nz)
            if L < 1e-12:
                return (0.0, 1.0, 0.0)
            return (nx / L, ny / L, nz / L)

        # --- Surface. Emitted DOUBLE-SIDED so a cut ellipsoid stays visible
        #     when the camera looks into the cut. ---
        for i in range(stacks):
            phi_hi = p1 - (p1 - p0) * (i / stacks)
            phi_lo = p1 - (p1 - p0) * ((i + 1) / stacks)
            for j in range(slices):
                th_lo = t0 + (t1 - t0) * (j / slices)
                th_hi = t0 + (t1 - t0) * ((j + 1) / slices)
                v_tl = P(phi_hi, th_lo)
                v_bl = P(phi_lo, th_lo)
                v_br = P(phi_lo, th_hi)
                v_tr = P(phi_hi, th_hi)
                n_tl = N(phi_hi, th_lo)
                n_bl = N(phi_lo, th_lo)
                n_br = N(phi_lo, th_hi)
                n_tr = N(phi_hi, th_hi)
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    v_tl,
                    v_bl,
                    v_br,
                    n_tl,
                    n_bl,
                    n_br,
                    C,
                )
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    v_br,
                    v_tr,
                    v_tl,
                    n_br,
                    n_tr,
                    n_tl,
                    C,
                )

        if caps:
            # --- Theta caps (meridional cuts) ---
            theta_span = theta_range[1] - theta_range[0]
            theta_full = abs(theta_span) >= 360 - 1e-6
            if not theta_full:
                for t_cap in (t0, t1):
                    for i in range(stacks):
                        phi_hi = p1 - (p1 - p0) * (i / stacks)
                        phi_lo = p1 - (p1 - p0) * ((i + 1) / stacks)
                        self.colors = _emit_tri2_flat(
                            self.vertices,
                            self.normals,
                            self.colors,
                            (0.0, 0.0, 0.0),
                            P(phi_lo, t_cap),
                            P(phi_hi, t_cap),
                            C,
                        )

            # --- Phi caps (latitudinal cuts that stop short of the poles) ---
            if phi_range[0] > -90 + 1e-6:
                center = (0.0, ry * math.sin(p0), 0.0)
                for j in range(slices):
                    th_a = t0 + (t1 - t0) * (j / slices)
                    th_b = t0 + (t1 - t0) * ((j + 1) / slices)
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        center,
                        P(p0, th_a),
                        P(p0, th_b),
                        C,
                    )
            if phi_range[1] < 90 - 1e-6:
                center = (0.0, ry * math.sin(p1), 0.0)
                for j in range(slices):
                    th_a = t0 + (t1 - t0) * (j / slices)
                    th_b = t0 + (t1 - t0) * ((j + 1) / slices)
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        center,
                        P(p1, th_a),
                        P(p1, th_b),
                        C,
                    )

        self.indices = list(range(len(self.vertices) // 3))


class Torus(Primitive):
    """
    Torus with an optional partial sweep. Angles in DEGREES.
      major_radius   : distance from the center of the tube to the center of the torus
      minor_radius   : radius of the tube
      major_range    : (u_start, u_end) in degrees; how much of the big ring to
                       draw. Default (0, 360) is a closed donut.
      minor_range    : (v_start, v_end) in degrees; tube cross-section sweep.
      color          : single (r, g, b, a) byte tuple applied uniformly
      caps           : if True, fill partial sweeps with flat caps

    Surface normal at (u, v) is the outward direction from the tube center,
    analytically = (cos v * cos u, sin v, -cos v * sin u), unit length.

    The lateral surface is emitted double-sided so the torus stays visible
    from any camera angle, including looking into the cut of a swept torus.
    """

    def __init__(
        self,
        major_radius: float = 1.0,
        minor_radius: float = 0.3,
        major_segments: int = 40,
        minor_segments: int = 20,
        major_range: tuple[float, float] = (0, 360),
        minor_range: tuple[float, float] = (0, 360),
        color: Color = (120, 180, 120, 255),
        caps: bool = True,
    ) -> None:
        R = major_radius
        r = minor_radius
        u0 = math.radians(major_range[0])
        u1 = math.radians(major_range[1])
        v0 = math.radians(minor_range[0])
        v1 = math.radians(minor_range[1])
        C = color

        self.vertices = []
        self.normals = []
        self.indices = []
        self.colors = ()

        def P(u: float, v: float) -> Point3:
            cu, su = math.cos(u), math.sin(u)
            cv, sv = math.cos(v), math.sin(v)
            rr = R + r * cv
            return (rr * cu, r * sv, -rr * su)

        def N(u: float, v: float) -> Point3:
            cu, su = math.cos(u), math.sin(u)
            cv, sv = math.cos(v), math.sin(v)
            return (cv * cu, sv, -cv * su)

        # --- Surface. Emitted DOUBLE-SIDED. ---
        for i in range(major_segments):
            u_a = u0 + (u1 - u0) * i / major_segments
            u_b = u0 + (u1 - u0) * (i + 1) / major_segments
            for j in range(minor_segments):
                v_a = v0 + (v1 - v0) * j / minor_segments
                v_b = v0 + (v1 - v0) * (j + 1) / minor_segments
                p00 = P(u_a, v_a)
                p10 = P(u_b, v_a)
                p11 = P(u_b, v_b)
                p01 = P(u_a, v_b)
                n00 = N(u_a, v_a)
                n10 = N(u_b, v_a)
                n11 = N(u_b, v_b)
                n01 = N(u_a, v_b)
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    p00,
                    p10,
                    p11,
                    n00,
                    n10,
                    n11,
                    C,
                )
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    p00,
                    p11,
                    p01,
                    n00,
                    n11,
                    n01,
                    C,
                )

        if caps:
            major_full = abs(u1 - u0) >= 2 * math.pi - 1e-6
            minor_full = abs(v1 - v0) >= 2 * math.pi - 1e-6

            # --- Major-sweep caps: fill the open tube cross-sections ---
            if not major_full:
                for u_cap in (u0, u1):
                    cu, su = math.cos(u_cap), math.sin(u_cap)
                    center = (R * cu, 0.0, -R * su)
                    for j in range(minor_segments):
                        v_a = v0 + (v1 - v0) * j / minor_segments
                        v_b = v0 + (v1 - v0) * (j + 1) / minor_segments
                        self.colors = _emit_tri2_flat(
                            self.vertices,
                            self.normals,
                            self.colors,
                            center,
                            P(u_cap, v_a),
                            P(u_cap, v_b),
                            C,
                        )

            # --- Minor-sweep cap: flat ribbon along the longitudinal slit ---
            if not minor_full:
                for i in range(major_segments):
                    u_a = u0 + (u1 - u0) * i / major_segments
                    u_b = u0 + (u1 - u0) * (i + 1) / major_segments
                    p_a0 = P(u_a, v0)
                    p_a1 = P(u_a, v1)
                    p_b0 = P(u_b, v0)
                    p_b1 = P(u_b, v1)
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        p_a0,
                        p_b0,
                        p_b1,
                        C,
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        p_a0,
                        p_b1,
                        p_a1,
                        C,
                    )

        self.indices = list(range(len(self.vertices) // 3))


class Frustum(Primitive):
    """
    Frustum (general truncated cone), optionally hollow.

      bottom_radius / top_radius : outer radius at h=0 and h=1.
          top_radius=0          -> cone
          bottom_radius==top_r  -> cylinder
      inner_bottom_radius / inner_top_radius : inner radius. If > 0 the frustum
          becomes a shell with an inner wall; bottom/top caps become annular
          rings and radial-cut caps become trapezoids.
      height      : total extent along Y. Centered at the origin.
      slices      : circumferential segments.
      theta_range : radial sweep in DEGREES, for angular cuts.
      color       : single (r, g, b, a) byte tuple applied uniformly.
      caps        : if True, draw bottom/top and radial-cut caps.

    Lateral normal derivation: for a point P(theta, h) = (R(h) cos th, (h-0.5)H,
    -R(h) sin th), the analytical outward normal via cross(dP/dth, dP/dh) is
      N = (H*cos th, Rb-Rt, -H*sin th) / sqrt(H^2 + (Rb-Rt)^2).
    For a cylinder (Rb==Rt) this collapses to (cos th, 0, -sin th); for a cone
    it tilts to account for the slant.
    """

    def __init__(
        self,
        bottom_radius: float = 1.0,
        top_radius: float = 0.5,
        height: float = 1.0,
        slices: int = 30,
        theta_range: tuple[float, float] = (0, 360),
        inner_bottom_radius: float = 0.0,
        inner_top_radius: float = 0.0,
        color: Color = (120, 140, 200, 255),
        caps: bool = True,
    ) -> None:
        Rb = bottom_radius
        Rt = top_radius
        Rbi = inner_bottom_radius
        Rti = inner_top_radius
        H = height
        t0 = math.radians(theta_range[0])
        t1 = math.radians(theta_range[1])
        C = color

        self.vertices = []
        self.normals = []
        self.indices = []
        self.colors = ()

        def P_outer(theta: float, h: float) -> Point3:
            R = Rb * (1.0 - h) + Rt * h
            return (R * math.cos(theta), (h - 0.5) * H, -R * math.sin(theta))

        def P_inner(theta: float, h: float) -> Point3:
            R = Rbi * (1.0 - h) + Rti * h
            return (R * math.cos(theta), (h - 0.5) * H, -R * math.sin(theta))

        # Outer / inner slant-aware normals (h-independent: normal only varies
        # with theta because the slant is constant along the generator).
        len_outer = math.sqrt(H * H + (Rb - Rt) * (Rb - Rt))
        len_inner = math.sqrt(H * H + (Rbi - Rti) * (Rbi - Rti))

        def N_outer(theta: float) -> Point3:
            if len_outer < 1e-12:
                return (math.cos(theta), 0.0, -math.sin(theta))
            return (
                H * math.cos(theta) / len_outer,
                (Rb - Rt) / len_outer,
                -H * math.sin(theta) / len_outer,
            )

        def N_inner(theta: float) -> Point3:
            # Inner wall's visible side faces the bore -> opposite of the
            # outward-of-cylinder direction.
            if len_inner < 1e-12:
                return (-math.cos(theta), 0.0, math.sin(theta))
            return (
                -H * math.cos(theta) / len_inner,
                (Rti - Rbi) / len_inner,
                H * math.sin(theta) / len_inner,
            )

        has_inner = (Rbi > 1e-6) or (Rti > 1e-6)

        def th(j: int) -> float:
            return t0 + (t1 - t0) * (j / slices)

        # --- Outer lateral surface. DOUBLE-SIDED with analytical normals. ---
        for j in range(slices):
            th_a, th_b = th(j), th(j + 1)
            p00 = P_outer(th_a, 0.0)
            p10 = P_outer(th_b, 0.0)
            p11 = P_outer(th_b, 1.0)
            p01 = P_outer(th_a, 1.0)
            n_a = N_outer(th_a)
            n_b = N_outer(th_b)
            self.colors = _emit_tri2_smooth(
                self.vertices,
                self.normals,
                self.colors,
                p00,
                p10,
                p11,
                n_a,
                n_b,
                n_b,
                C,
            )
            self.colors = _emit_tri2_smooth(
                self.vertices,
                self.normals,
                self.colors,
                p00,
                p11,
                p01,
                n_a,
                n_b,
                n_a,
                C,
            )

        # --- Inner lateral surface. Also double-sided. ---
        if has_inner:
            for j in range(slices):
                th_a, th_b = th(j), th(j + 1)
                p00 = P_inner(th_a, 0.0)
                p10 = P_inner(th_b, 0.0)
                p11 = P_inner(th_b, 1.0)
                p01 = P_inner(th_a, 1.0)
                n_a = N_inner(th_a)
                n_b = N_inner(th_b)
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    p00,
                    p10,
                    p11,
                    n_a,
                    n_b,
                    n_b,
                    C,
                )
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    p00,
                    p11,
                    p01,
                    n_a,
                    n_b,
                    n_a,
                    C,
                )

        if caps:
            # --- Bottom / top rings (annular when hollow, disks when solid) ---
            if Rb > 1e-6 or Rbi > 1e-6:
                for j in range(slices):
                    th_a, th_b = th(j), th(j + 1)
                    o_a = P_outer(th_a, 0.0)
                    o_b = P_outer(th_b, 0.0)
                    i_a = P_inner(th_a, 0.0)
                    i_b = P_inner(th_b, 0.0)
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        i_a,
                        o_a,
                        o_b,
                        C,
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        i_a,
                        o_b,
                        i_b,
                        C,
                    )
            if Rt > 1e-6 or Rti > 1e-6:
                for j in range(slices):
                    th_a, th_b = th(j), th(j + 1)
                    o_a = P_outer(th_a, 1.0)
                    o_b = P_outer(th_b, 1.0)
                    i_a = P_inner(th_a, 1.0)
                    i_b = P_inner(th_b, 1.0)
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        i_a,
                        o_a,
                        o_b,
                        C,
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        i_a,
                        o_b,
                        i_b,
                        C,
                    )

            # --- Radial cut caps (partial theta sweep): trapezoid between
            #     inner and outer walls, at each exposed meridian.
            theta_full = abs(theta_range[1] - theta_range[0]) >= 360 - 1e-6
            if not theta_full:
                for th_cap in (t0, t1):
                    i_bot = P_inner(th_cap, 0.0)
                    i_top = P_inner(th_cap, 1.0)
                    o_bot = P_outer(th_cap, 0.0)
                    o_top = P_outer(th_cap, 1.0)
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        i_bot,
                        o_bot,
                        o_top,
                        C,
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        i_bot,
                        o_top,
                        i_top,
                        C,
                    )

        self.indices = list(range(len(self.vertices) // 3))


class Axes(Primitive):
    """
    Coordinate-axis reference built out of GL_LINES. Not a cut-surface primitive;
    its purpose is to help eyeball positions while modelling.

      length    : how far each axis extends from the origin (in both directions)
      tick      : how often a unit tick mark is drawn along each axis
      tick_size : half-size of each tick cross in world units

    Colour convention:
      X = red, Y = green, Z = blue.
      The POSITIVE half of each axis is drawn in full saturation; the NEGATIVE
      half is dimmed to about 30% brightness so you can tell them apart at a
      glance. Each positive tip also has a small 3D arrowhead.
      Ticks at integer positions are drawn in white (positive) / grey (negative).

    Renders as a line-primitive; the renderer picks this up via the ``.mode``
    attribute set to GL_LINES. ``lit = False`` routes it through the unlit
    passthrough shader so axes/gizmos stay at their literal vertex color.
    """

    mode: int = GL_LINES
    lit: bool = False

    def __init__(
        self, length: float = 5, tick: float = 1, tick_size: float = 0.08
    ) -> None:
        self.vertices = []
        self.normals = []
        self.colors = ()
        self.indices = []

        def seg(a: Point3, b: Point3, c: Color) -> None:
            self.vertices.extend((a[0], a[1], a[2], b[0], b[1], b[2]))
            self.colors += c + c

        POS_X = (255, 40, 40, 255)
        NEG_X = (90, 20, 20, 255)
        POS_Y = (40, 255, 40, 255)
        NEG_Y = (20, 90, 20, 255)
        POS_Z = (40, 80, 255, 255)
        NEG_Z = (20, 30, 90, 255)
        TICK_POS = (230, 230, 230, 255)
        TICK_NEG = (110, 110, 110, 255)

        L = float(length)

        # Main axis lines (split at origin so +/- can be coloured differently)
        seg((0, 0, 0), (L, 0, 0), POS_X)
        seg((0, 0, 0), (-L, 0, 0), NEG_X)
        seg((0, 0, 0), (0, L, 0), POS_Y)
        seg((0, 0, 0), (0, -L, 0), NEG_Y)
        seg((0, 0, 0), (0, 0, L), POS_Z)
        seg((0, 0, 0), (0, 0, -L), NEG_Z)

        # Arrowhead at each positive tip (small 3D cross of back-pointing spokes)
        ah = tick_size * 3.0  # arrow length
        aw = tick_size * 1.5  # arrow half-width
        # +X arrowhead
        tip = (L, 0, 0)
        back = (L - ah, 0, 0)
        seg(tip, (back[0], aw, 0), POS_X)
        seg(tip, (back[0], -aw, 0), POS_X)
        seg(tip, (back[0], 0, aw), POS_X)
        seg(tip, (back[0], 0, -aw), POS_X)
        # +Y arrowhead
        tip = (0, L, 0)
        back = (0, L - ah, 0)
        seg(tip, (aw, back[1], 0), POS_Y)
        seg(tip, (-aw, back[1], 0), POS_Y)
        seg(tip, (0, back[1], aw), POS_Y)
        seg(tip, (0, back[1], -aw), POS_Y)
        # +Z arrowhead
        tip = (0, 0, L)
        back = (0, 0, L - ah)
        seg(tip, (aw, 0, back[2]), POS_Z)
        seg(tip, (-aw, 0, back[2]), POS_Z)
        seg(tip, (0, aw, back[2]), POS_Z)
        seg(tip, (0, -aw, back[2]), POS_Z)

        # Unit tick marks (small crosses perpendicular to each axis)
        k = 1
        while k * tick <= length + 1e-6:
            d = k * tick
            c_pos = TICK_POS
            c_neg = TICK_NEG
            # X axis ticks (in Y and Z)
            seg((d, -tick_size, 0), (d, tick_size, 0), c_pos)
            seg((d, 0, -tick_size), (d, 0, tick_size), c_pos)
            seg((-d, -tick_size, 0), (-d, tick_size, 0), c_neg)
            seg((-d, 0, -tick_size), (-d, 0, tick_size), c_neg)
            # Y axis ticks (in X and Z)
            seg((-tick_size, d, 0), (tick_size, d, 0), c_pos)
            seg((0, d, -tick_size), (0, d, tick_size), c_pos)
            seg((-tick_size, -d, 0), (tick_size, -d, 0), c_neg)
            seg((0, -d, -tick_size), (0, -d, tick_size), c_neg)
            # Z axis ticks (in X and Y)
            seg((-tick_size, 0, d), (tick_size, 0, d), c_pos)
            seg((0, -tick_size, d), (0, tick_size, d), c_pos)
            seg((-tick_size, 0, -d), (tick_size, 0, -d), c_neg)
            seg((0, -tick_size, -d), (0, tick_size, -d), c_neg)
            k += 1

        self.indices = list(range(len(self.vertices) // 3))
