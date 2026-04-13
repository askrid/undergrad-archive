from __future__ import annotations

import math
from typing import Any

import pyglet
from pyglet.math import Mat4
from pyglet.gl import GL_TRIANGLES, GL_LINES

Point3 = tuple[float, float, float]
Color = tuple[int, int, int, int]


class CustomGroup(pyglet.graphics.Group):
    """Per-shape render group. Sets the model matrix uniform before draw."""

    def __init__(self, transform_mat: Mat4, order: int, shader_program: Any) -> None:
        super().__init__(order)
        self.shader_program = shader_program
        self.transform_mat = transform_mat
        self.indexed_vertices_list: Any = None
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
    """Base for geometry consumed by RenderWindow.add_shape."""

    mode: int = GL_TRIANGLES
    lit: bool = True
    vertices: list[float]
    normals: list[float]
    indices: list[int]
    colors: tuple[int, ...]


# -- Triangle emission helpers (double-sided with correct normals) --


def _face_normal(a: Point3, b: Point3, c: Point3) -> Point3:
    ux, uy, uz = b[0] - a[0], b[1] - a[1], b[2] - a[2]
    vx, vy, vz = c[0] - a[0], c[1] - a[1], c[2] - a[2]
    nx, ny, nz = uy * vz - uz * vy, uz * vx - ux * vz, ux * vy - uy * vx
    L = math.sqrt(nx * nx + ny * ny + nz * nz)
    return (nx / L, ny / L, nz / L) if L > 1e-12 else (0.0, 0.0, 0.0)


def _v3(p: Point3) -> tuple[float, float, float]:
    return p[0], p[1], p[2]


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
    """Emit triangle double-sided: front (a,b,c) + back (a,c,b) with negated normals."""
    verts.extend((*a, *b, *c, *a, *c, *b))
    norms.extend(
        (*na, *nb, *nc, *(-x for x in na), *(-x for x in nc), *(-x for x in nb))
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
    return _emit_tri2(verts, norms, cols, a, b, c, na, nb, nc, col, col, col)


class Cube(Primitive):
    """Axis-aligned box centered at origin. 24 verts (per-face normals)."""

    def __init__(
        self, size: float | Point3 = 1.0, color: Color = (200, 200, 200, 255)
    ) -> None:
        if isinstance(size, (int, float)):
            sx = sy = sz = float(size)
        else:
            sx, sy, sz = float(size[0]), float(size[1]), float(size[2])
        hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5

        faces: list[tuple[Point3, list[Point3]]] = [
            ((1, 0, 0), [(hx, -hy, -hz), (hx, hy, -hz), (hx, hy, hz), (hx, -hy, hz)]),
            (
                (-1, 0, 0),
                [(-hx, -hy, -hz), (-hx, -hy, hz), (-hx, hy, hz), (-hx, hy, -hz)],
            ),
            ((0, 1, 0), [(hx, hy, hz), (hx, hy, -hz), (-hx, hy, -hz), (-hx, hy, hz)]),
            (
                (0, -1, 0),
                [(hx, -hy, hz), (-hx, -hy, hz), (-hx, -hy, -hz), (hx, -hy, -hz)],
            ),
            ((0, 0, 1), [(-hx, -hy, hz), (hx, -hy, hz), (hx, hy, hz), (-hx, hy, hz)]),
            (
                (0, 0, -1),
                [(hx, -hy, -hz), (-hx, -hy, -hz), (-hx, hy, -hz), (hx, hy, -hz)],
            ),
        ]

        self.vertices: list[float] = []
        self.normals: list[float] = []
        self.indices: list[int] = []
        for n, quad in faces:
            base = len(self.vertices) // 3
            for p in quad:
                self.vertices.extend(p)
                self.normals.extend(n)
            self.indices.extend([base, base + 1, base + 2, base, base + 2, base + 3])
        self.colors = color * 24


class Ellipsoid(Primitive):
    """Parametric ellipsoid with angular cuts. Analytical gradient normals."""

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
        rx, ry, rz = radius
        t0, t1 = math.radians(theta_range[0]), math.radians(theta_range[1])
        p0, p1 = math.radians(phi_range[0]), math.radians(phi_range[1])
        C = color

        self.vertices: list[float] = []
        self.normals: list[float] = []
        self.colors: tuple[int, ...] = ()

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
            return (nx / L, ny / L, nz / L) if L > 1e-12 else (0.0, 1.0, 0.0)

        # Surface (double-sided)
        for i in range(stacks):
            phi_hi = p1 - (p1 - p0) * (i / stacks)
            phi_lo = p1 - (p1 - p0) * ((i + 1) / stacks)
            for j in range(slices):
                th_lo = t0 + (t1 - t0) * (j / slices)
                th_hi = t0 + (t1 - t0) * ((j + 1) / slices)
                v_tl, v_bl = P(phi_hi, th_lo), P(phi_lo, th_lo)
                v_br, v_tr = P(phi_lo, th_hi), P(phi_hi, th_hi)
                n_tl, n_bl = N(phi_hi, th_lo), N(phi_lo, th_lo)
                n_br, n_tr = N(phi_lo, th_hi), N(phi_hi, th_hi)
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
            theta_full = abs(theta_range[1] - theta_range[0]) >= 360 - 1e-6
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
    """Torus with optional partial sweeps. Analytical surface normals."""

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
        R, r = major_radius, minor_radius
        u0, u1 = math.radians(major_range[0]), math.radians(major_range[1])
        v0, v1 = math.radians(minor_range[0]), math.radians(minor_range[1])
        C = color

        self.vertices: list[float] = []
        self.normals: list[float] = []
        self.colors: tuple[int, ...] = ()

        def P(u: float, v: float) -> Point3:
            cu, su, cv, sv = math.cos(u), math.sin(u), math.cos(v), math.sin(v)
            rr = R + r * cv
            return (rr * cu, r * sv, -rr * su)

        def N(u: float, v: float) -> Point3:
            cu, su, cv, sv = math.cos(u), math.sin(u), math.cos(v), math.sin(v)
            return (cv * cu, sv, -cv * su)

        # Surface (double-sided)
        for i in range(major_segments):
            u_a = u0 + (u1 - u0) * i / major_segments
            u_b = u0 + (u1 - u0) * (i + 1) / major_segments
            for j in range(minor_segments):
                v_a = v0 + (v1 - v0) * j / minor_segments
                v_b = v0 + (v1 - v0) * (j + 1) / minor_segments
                p00, p10, p11, p01 = P(u_a, v_a), P(u_b, v_a), P(u_b, v_b), P(u_a, v_b)
                n00, n10, n11, n01 = N(u_a, v_a), N(u_b, v_a), N(u_b, v_b), N(u_a, v_b)
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

            if not minor_full:
                for i in range(major_segments):
                    u_a = u0 + (u1 - u0) * i / major_segments
                    u_b = u0 + (u1 - u0) * (i + 1) / major_segments
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        P(u_a, v0),
                        P(u_b, v0),
                        P(u_b, v1),
                        C,
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices,
                        self.normals,
                        self.colors,
                        P(u_a, v0),
                        P(u_b, v1),
                        P(u_a, v1),
                        C,
                    )

        self.indices = list(range(len(self.vertices) // 3))


class Frustum(Primitive):
    """Truncated cone, optionally hollow. Slant-aware analytical normals."""

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
        Rb, Rt, Rbi, Rti, H = (
            bottom_radius,
            top_radius,
            inner_bottom_radius,
            inner_top_radius,
            height,
        )
        t0, t1 = math.radians(theta_range[0]), math.radians(theta_range[1])
        C = color

        self.vertices: list[float] = []
        self.normals: list[float] = []
        self.colors: tuple[int, ...] = ()

        def P_outer(theta: float, h: float) -> Point3:
            R = Rb * (1.0 - h) + Rt * h
            return (R * math.cos(theta), (h - 0.5) * H, -R * math.sin(theta))

        def P_inner(theta: float, h: float) -> Point3:
            R = Rbi * (1.0 - h) + Rti * h
            return (R * math.cos(theta), (h - 0.5) * H, -R * math.sin(theta))

        len_outer = math.sqrt(H * H + (Rb - Rt) ** 2)
        len_inner = math.sqrt(H * H + (Rbi - Rti) ** 2)

        def N_outer(theta: float) -> Point3:
            if len_outer < 1e-12:
                return (math.cos(theta), 0.0, -math.sin(theta))
            return (
                H * math.cos(theta) / len_outer,
                (Rb - Rt) / len_outer,
                -H * math.sin(theta) / len_outer,
            )

        def N_inner(theta: float) -> Point3:
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

        def _lateral(P_fn, N_fn):
            for j in range(slices):
                th_a, th_b = th(j), th(j + 1)
                p00, p10 = P_fn(th_a, 0.0), P_fn(th_b, 0.0)
                p11, p01 = P_fn(th_b, 1.0), P_fn(th_a, 1.0)
                na, nb = N_fn(th_a), N_fn(th_b)
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    p00,
                    p10,
                    p11,
                    na,
                    nb,
                    nb,
                    C,
                )
                self.colors = _emit_tri2_smooth(
                    self.vertices,
                    self.normals,
                    self.colors,
                    p00,
                    p11,
                    p01,
                    na,
                    nb,
                    na,
                    C,
                )

        _lateral(P_outer, N_outer)
        if has_inner:
            _lateral(P_inner, N_inner)

        if caps:
            # Bottom/top annular rings
            def _ring(h: float):
                for j in range(slices):
                    th_a, th_b = th(j), th(j + 1)
                    oa, ob = P_outer(th_a, h), P_outer(th_b, h)
                    ia, ib = P_inner(th_a, h), P_inner(th_b, h)
                    self.colors = _emit_tri2_flat(
                        self.vertices, self.normals, self.colors, ia, oa, ob, C
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices, self.normals, self.colors, ia, ob, ib, C
                    )

            if Rb > 1e-6 or Rbi > 1e-6:
                _ring(0.0)
            if Rt > 1e-6 or Rti > 1e-6:
                _ring(1.0)

            # Radial cut caps
            theta_full = abs(theta_range[1] - theta_range[0]) >= 360 - 1e-6
            if not theta_full:
                for th_cap in (t0, t1):
                    i_bot, i_top = P_inner(th_cap, 0.0), P_inner(th_cap, 1.0)
                    o_bot, o_top = P_outer(th_cap, 0.0), P_outer(th_cap, 1.0)
                    self.colors = _emit_tri2_flat(
                        self.vertices, self.normals, self.colors, i_bot, o_bot, o_top, C
                    )
                    self.colors = _emit_tri2_flat(
                        self.vertices, self.normals, self.colors, i_bot, o_top, i_top, C
                    )

        self.indices = list(range(len(self.vertices) // 3))


class Axes(Primitive):
    """Wireframe RGB coordinate axes with tick marks. Unlit."""

    mode: int = GL_LINES
    lit: bool = False

    def __init__(
        self, length: float = 5, tick: float = 1, tick_size: float = 0.08
    ) -> None:
        self.vertices: list[float] = []
        self.normals: list[float] = []
        self.colors: tuple[int, ...] = ()
        self.indices: list[int] = []

        def seg(a: Point3, b: Point3, c: Color) -> None:
            self.vertices.extend((*a, *b))
            self.colors += c + c

        POS_X, NEG_X = (255, 40, 40, 255), (90, 20, 20, 255)
        POS_Y, NEG_Y = (40, 255, 40, 255), (20, 90, 20, 255)
        POS_Z, NEG_Z = (40, 80, 255, 255), (20, 30, 90, 255)
        TICK_POS, TICK_NEG = (230, 230, 230, 255), (110, 110, 110, 255)
        L = float(length)

        # Main axes (split at origin for +/- coloring)
        seg((0, 0, 0), (L, 0, 0), POS_X)
        seg((0, 0, 0), (-L, 0, 0), NEG_X)
        seg((0, 0, 0), (0, L, 0), POS_Y)
        seg((0, 0, 0), (0, -L, 0), NEG_Y)
        seg((0, 0, 0), (0, 0, L), POS_Z)
        seg((0, 0, 0), (0, 0, -L), NEG_Z)

        # Arrowheads at positive tips
        ah, aw = tick_size * 3.0, tick_size * 1.5
        for tip, back_ax, col in [
            ((L, 0, 0), 0, POS_X),
            ((0, L, 0), 1, POS_Y),
            ((0, 0, L), 2, POS_Z),
        ]:
            for axis in range(3):
                if axis == back_ax:
                    continue
                for sign in (aw, -aw):
                    pt = list(tip)
                    pt[back_ax] -= ah
                    pt[axis] = sign
                    seg(tip, tuple(pt), col)  # type: ignore

        # Tick marks
        k = 1
        while k * tick <= length + 1e-6:
            d = k * tick
            ts = tick_size
            seg((d, -ts, 0), (d, ts, 0), TICK_POS)
            seg((d, 0, -ts), (d, 0, ts), TICK_POS)
            seg((-d, -ts, 0), (-d, ts, 0), TICK_NEG)
            seg((-d, 0, -ts), (-d, 0, ts), TICK_NEG)
            seg((-ts, d, 0), (ts, d, 0), TICK_POS)
            seg((0, d, -ts), (0, d, ts), TICK_POS)
            seg((-ts, -d, 0), (ts, -d, 0), TICK_NEG)
            seg((0, -d, -ts), (0, -d, ts), TICK_NEG)
            seg((-ts, 0, d), (ts, 0, d), TICK_POS)
            seg((0, -ts, d), (0, ts, d), TICK_POS)
            seg((-ts, 0, -d), (ts, 0, -d), TICK_NEG)
            seg((0, -ts, -d), (0, ts, -d), TICK_NEG)
            k += 1

        self.indices = list(range(len(self.vertices) // 3))
