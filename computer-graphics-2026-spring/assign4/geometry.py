from __future__ import annotations

import torch

INF = 1.0e30
EPS = 1.0e-4


def normalize(v: torch.Tensor, eps: float = 1e-9) -> torch.Tensor:
    return v / v.norm(dim=-1, keepdim=True).clamp_min(eps)


def intersect_spheres(ro, rd, centers, radii):
    """Closest sphere hit per ray. Returns (t[N], outward normal[N,3], idx[N])."""
    N = ro.shape[0]
    if centers.shape[0] == 0:
        return (
            ro.new_full((N,), INF),
            ro.new_zeros((N, 3)),
            ro.new_zeros((N,), dtype=torch.long),
        )
    oc = ro[:, None, :] - centers[None, :, :]  # [N,S,3]
    b = (oc * rd[:, None, :]).sum(-1)  # [N,S]
    c = (oc * oc).sum(-1) - radii[None, :] ** 2  # [N,S]
    disc = b * b - c
    valid = disc > 0.0
    sq = torch.sqrt(disc.clamp_min(0.0))
    t0 = -b - sq
    t1 = -b + sq
    t = torch.where(t0 > EPS, t0, t1)
    t = torch.where(valid & (t > EPS), t, ro.new_full((), INF))
    tmin, idx = t.min(dim=1)  # [N]
    p = ro + tmin[:, None] * rd
    ctr = centers[idx]
    n = normalize(p - ctr)
    return tmin, n, idx


def intersect_quads(ro, rd, q0, e1, e2, qn):
    """Closest quad hit per ray. Returns (t[N], normal[N,3], quad_idx[N])."""
    N = ro.shape[0]
    if q0.shape[0] == 0:
        return (
            ro.new_full((N,), INF),
            ro.new_zeros((N, 3)),
            ro.new_zeros((N,), dtype=torch.long),
        )
    denom = (rd[:, None, :] * qn[None]).sum(-1)  # [N,Q]
    parallel = denom.abs() < 1e-8
    diff = q0[None] - ro[:, None, :]  # [N,Q,3]
    t = (diff * qn[None]).sum(-1) / torch.where(parallel, torch.ones_like(denom), denom)
    p = ro[:, None, :] + t[..., None] * rd[:, None, :]  # [N,Q,3]
    rel = p - q0[None]  # [N,Q,3]
    a = (rel * e1[None]).sum(-1) / (e1 * e1).sum(-1)[None]
    bb = (rel * e2[None]).sum(-1) / (e2 * e2).sum(-1)[None]
    inside = (a >= 0) & (a <= 1) & (bb >= 0) & (bb <= 1)
    ok = inside & (~parallel) & (t > EPS)
    t = torch.where(ok, t, ro.new_full((), INF))
    tmin, idx = t.min(dim=1)
    n = qn[idx]
    return tmin, n, idx


def intersect_triangles_brute(ro, rd, V, Nv, ray_chunk=4096, tri_chunk=2048):
    """Closest triangle hit per ray, tiled to bound memory (BVH oracle).

    Returns (t[N], interpolated normal[N,3], tri_idx[N]).
    """
    N = ro.shape[0]
    T = V.shape[0]
    out_t = ro.new_full((N,), INF)
    out_n = ro.new_zeros((N, 3))
    out_i = ro.new_zeros((N,), dtype=torch.long)
    v0a, v1a, v2a = V[:, 0], V[:, 1], V[:, 2]
    e1a, e2a = v1a - v0a, v2a - v0a
    for rs in range(0, N, ray_chunk):
        re = min(rs + ray_chunk, N)
        o = ro[rs:re]
        d = rd[rs:re]
        bt = o.new_full((re - rs,), INF)
        bu = o.new_zeros((re - rs,))
        bv = o.new_zeros((re - rs,))
        bi = o.new_zeros((re - rs,), dtype=torch.long)
        for ts in range(0, T, tri_chunk):
            te = min(ts + tri_chunk, T)
            v0 = v0a[ts:te]
            e1 = e1a[ts:te]
            e2 = e2a[ts:te]
            pvec = torch.cross(d[:, None, :], e2[None], dim=-1)  # [n,c,3]
            det = (e1[None] * pvec).sum(-1)  # [n,c]
            mask = det.abs() > 1e-9
            inv = 1.0 / torch.where(mask, det, torch.ones_like(det))
            tvec = o[:, None, :] - v0[None]
            u = (tvec * pvec).sum(-1) * inv
            qvec = torch.cross(tvec, e1[None], dim=-1)
            vv = (d[:, None, :] * qvec).sum(-1) * inv
            tt = (e2[None] * qvec).sum(-1) * inv
            ok = mask & (u >= 0) & (vv >= 0) & (u + vv <= 1) & (tt > EPS)
            tt = torch.where(ok, tt, o.new_full((), INF))
            tmin, j = tt.min(dim=1)  # [n]
            better = tmin < bt
            bt = torch.where(better, tmin, bt)
            jg = j + ts
            bi = torch.where(better, jg, bi)
            ar = torch.arange(re - rs, device=o.device)
            bu = torch.where(better, u[ar, j], bu)
            bv = torch.where(better, vv[ar, j], bv)
        out_t[rs:re] = bt
        out_i[rs:re] = bi
        # interpolate shading normal from barycentric coords
        n0 = Nv[bi, 0]
        n1 = Nv[bi, 1]
        n2 = Nv[bi, 2]
        w0 = (1.0 - bu - bv)[:, None]
        n = w0 * n0 + bu[:, None] * n1 + bv[:, None] * n2
        out_n[rs:re] = normalize(n)
    return out_t, out_n, out_i


def ray_aabb(ro, rd, inv_d, bmin, bmax):
    """
    Returns (hit[N] bool, tnear[N]). tnear is the entry distance (clamped >=0).
    """
    t0 = (bmin - ro) * inv_d
    t1 = (bmax - ro) * inv_d
    tsmall = torch.minimum(t0, t1)
    tbig = torch.maximum(t0, t1)
    tnear = tsmall.max(dim=-1).values
    tfar = tbig.min(dim=-1).values
    hit = (tfar >= tnear.clamp_min(0.0)) & (tfar >= 0.0)
    return hit, tnear
