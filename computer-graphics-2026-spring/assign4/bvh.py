"""BVH with CPU median-split build and GPU wavefront traversal.

Leaves hold a fixed-width block of triangle indices (padded with -1) so the GPU
can gather them without ragged indexing. Traversal is vectorized over all rays,
each carrying its own stack tensor. closest gives the nearest hit, occluded the
any-hit shadow test.
"""

from __future__ import annotations

import numpy as np
import torch

from geometry import EPS, INF, ray_aabb, normalize

LEAF_SIZE = 8
MAX_STACK = 64


def build_bvh(V: np.ndarray, leaf_size: int = LEAF_SIZE):
    """Build a median-split BVH over triangles V[T,3,3] into flat numpy arrays."""
    T = V.shape[0]
    tmin = V.min(axis=1)  # [T,3]
    tmax = V.max(axis=1)
    cent = V.mean(axis=1)

    nmin: list = []
    nmax: list = []
    nleft: list = []
    nright: list = []
    nleaf: list = []
    leaf_tris: list = []

    import sys

    sys.setrecursionlimit(1 << 20)

    def recurse(idx: np.ndarray) -> int:
        node_id = len(nmin)
        bmin = tmin[idx].min(axis=0)
        bmax = tmax[idx].max(axis=0)
        nmin.append(bmin)
        nmax.append(bmax)
        nleft.append(-1)
        nright.append(-1)
        nleaf.append(-1)
        if len(idx) <= leaf_size:
            lid = len(leaf_tris)
            pad = np.full(leaf_size, -1, dtype=np.int64)
            pad[: len(idx)] = idx
            leaf_tris.append(pad)
            nleaf[node_id] = lid
            return node_id
        c = cent[idx]
        axis = int(np.argmax(c.max(axis=0) - c.min(axis=0)))
        order = idx[np.argsort(c[:, axis], kind="stable")]
        mid = len(order) // 2
        left = recurse(order[:mid])
        right = recurse(order[mid:])
        nleft[node_id] = left
        nright[node_id] = right
        return node_id

    recurse(np.arange(T, dtype=np.int64))

    return {
        "nmin": np.asarray(nmin, dtype=np.float32),
        "nmax": np.asarray(nmax, dtype=np.float32),
        "nleft": np.asarray(nleft, dtype=np.int64),
        "nright": np.asarray(nright, dtype=np.int64),
        "nleaf": np.asarray(nleaf, dtype=np.int64),
        "leaf_tris": np.asarray(leaf_tris, dtype=np.int64).reshape(-1, leaf_size),
    }


def _tri_hit(o, d, v0, v1, v2):
    """Moller-Trumbore"""
    e1 = v1 - v0
    e2 = v2 - v0
    pv = torch.cross(d, e2, dim=-1)
    det = (e1 * pv).sum(-1)
    mask = det.abs() > 1e-9
    inv = 1.0 / torch.where(mask, det, torch.ones_like(det))
    tv = o - v0
    u = (tv * pv).sum(-1) * inv
    qv = torch.cross(tv, e1, dim=-1)
    v = (d * qv).sum(-1) * inv
    t = (e2 * qv).sum(-1) * inv
    ok = mask & (u >= 0) & (v >= 0) & (u + v <= 1) & (t > EPS)
    return ok, t, u, v


class BVHGpu:
    def __init__(self, data: dict, V: torch.Tensor, Nv: torch.Tensor):
        dev = V.device
        self.V = V  # [T,3,3]
        self.Nv = Nv  # [T,3,3]
        self.nmin = torch.as_tensor(data["nmin"], device=dev)
        self.nmax = torch.as_tensor(data["nmax"], device=dev)
        self.nleft = torch.as_tensor(data["nleft"], device=dev)
        self.nright = torch.as_tensor(data["nright"], device=dev)
        self.nleaf = torch.as_tensor(data["nleaf"], device=dev)
        self.leaf_tris = torch.as_tensor(data["leaf_tris"], device=dev)
        self.leaf_w = self.leaf_tris.shape[1]
        self.v0 = V[:, 0]
        self.v1 = V[:, 1]
        self.v2 = V[:, 2]

    @classmethod
    def build(cls, V: torch.Tensor, Nv: torch.Tensor, leaf_size: int = LEAF_SIZE):
        data = build_bvh(V.detach().cpu().numpy(), leaf_size=leaf_size)
        return cls(data, V, Nv)

    def _inv_dir(self, rd):
        safe = torch.where(rd.abs() < 1e-9, torch.full_like(rd, 1e-9), rd)
        return 1.0 / safe

    def closest(self, ro, rd, max_iter: int = 4096):
        """Closest triangle hit. Returns (t[N], normal[N,3], tri_idx[N])."""
        N = ro.shape[0]
        dev = ro.device
        inv_d = self._inv_dir(rd)
        best_t = ro.new_full((N,), INF)
        best_u = ro.new_zeros((N,))
        best_v = ro.new_zeros((N,))
        best_i = torch.full((N,), -1, dtype=torch.long, device=dev)

        stack = torch.zeros((N, MAX_STACK), dtype=torch.long, device=dev)
        sp = torch.ones((N,), dtype=torch.long, device=dev)  # root pushed at 0
        ar = torch.arange(N, device=dev)

        it = 0
        while bool((sp > 0).any()) and it < max_iter:
            it += 1
            active = sp > 0
            top = (sp - 1).clamp_min(0)
            node = stack[ar, top]
            sp = sp - active.long()

            bmin = self.nmin[node]
            bmax = self.nmax[node]
            hit, tnear = ray_aabb(ro, rd, inv_d, bmin, bmax)
            descend = active & hit & (tnear < best_t)
            isleaf = self.nleaf[node] >= 0

            leafmask = descend & isleaf
            if bool(leafmask.any()):
                lr = leafmask.nonzero(as_tuple=True)[0]
                lids = self.nleaf[node[lr]]
                tris = self.leaf_tris[lids]  # [K, leaf_w]
                o = ro[lr]
                d = rd[lr]
                for s in range(self.leaf_w):
                    ti = tris[:, s]
                    valid = ti >= 0
                    tic = ti.clamp_min(0)
                    ok, t, u, v = _tri_hit(
                        o, d, self.v0[tic], self.v1[tic], self.v2[tic]
                    )
                    better = valid & ok & (t < best_t[lr])
                    if bool(better.any()):
                        sel = lr[better]
                        best_t[sel] = t[better]
                        best_u[sel] = u[better]
                        best_v[sel] = v[better]
                        best_i[sel] = ti[better]

            intmask = descend & ~isleaf
            if bool(intmask.any()):
                ir = intmask.nonzero(as_tuple=True)[0]
                L = self.nleft[node[ir]]
                R = self.nright[node[ir]]
                spi = sp[ir]
                stack[ir, spi] = L
                stack[ir, spi + 1] = R
                sp[ir] = spi + 2

        # interpolate shading normals
        hitmask = best_i >= 0
        bi = best_i.clamp_min(0)
        n0 = self.Nv[bi, 0]
        n1 = self.Nv[bi, 1]
        n2 = self.Nv[bi, 2]
        w0 = (1.0 - best_u - best_v)[:, None]
        n = w0 * n0 + best_u[:, None] * n1 + best_v[:, None] * n2
        n = normalize(n)
        n = torch.where(hitmask[:, None], n, torch.zeros_like(n))
        return best_t, n, best_i

    def occluded(self, ro, rd, max_dist, max_iter: int = 4096):
        """Any-hit within (EPS, max_dist). Returns bool[N] of occluded rays."""
        N = ro.shape[0]
        dev = ro.device
        inv_d = self._inv_dir(rd)
        occ = torch.zeros((N,), dtype=torch.bool, device=dev)

        stack = torch.zeros((N, MAX_STACK), dtype=torch.long, device=dev)
        sp = torch.ones((N,), dtype=torch.long, device=dev)
        ar = torch.arange(N, device=dev)

        it = 0
        while bool((sp > 0).any()) and it < max_iter:
            it += 1
            active = (sp > 0) & (~occ)
            top = (sp - 1).clamp_min(0)
            node = stack[ar, top]
            sp = sp - (sp > 0).long()  # always pop to keep occluded rays draining

            bmin = self.nmin[node]
            bmax = self.nmax[node]
            hit, tnear = ray_aabb(ro, rd, inv_d, bmin, bmax)
            descend = active & hit & (tnear < max_dist)
            isleaf = self.nleaf[node] >= 0

            leafmask = descend & isleaf
            if bool(leafmask.any()):
                lr = leafmask.nonzero(as_tuple=True)[0]
                lids = self.nleaf[node[lr]]
                tris = self.leaf_tris[lids]
                o = ro[lr]
                d = rd[lr]
                md = max_dist[lr]
                for s in range(self.leaf_w):
                    ti = tris[:, s]
                    valid = ti >= 0
                    tic = ti.clamp_min(0)
                    ok, t, _, _ = _tri_hit(
                        o, d, self.v0[tic], self.v1[tic], self.v2[tic]
                    )
                    blocked = valid & ok & (t < md)
                    if bool(blocked.any()):
                        occ[lr[blocked]] = True

            intmask = descend & ~isleaf & (~occ)
            if bool(intmask.any()):
                ir = intmask.nonzero(as_tuple=True)[0]
                L = self.nleft[node[ir]]
                R = self.nright[node[ir]]
                spi = sp[ir]
                stack[ir, spi] = L
                stack[ir, spi + 1] = R
                sp[ir] = spi + 2

        return occ
