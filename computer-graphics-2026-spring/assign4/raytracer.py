"""Backward recursive (Whitted) ray tracer on the GPU."""

from __future__ import annotations

import torch

from geometry import normalize, intersect_spheres, intersect_quads, INF, EPS
from bvh import BVHGpu
from camera import Camera
from flatten import SceneData


def _refract(d, n, eta):
    """Refract incident dir d through normal n with ratio eta=n_in/n_out.

    n faces against d (toward the incoming ray). Returns (dir, valid) where
    valid is False on total internal reflection.
    """
    if eta.dim() == 1:
        eta = eta[:, None]
    cosi = (-d * n).sum(-1, keepdim=True).clamp(-1.0, 1.0)
    k = 1.0 - eta * eta * (1.0 - cosi * cosi)
    valid = (k >= 0.0).squeeze(-1)
    rd = eta * d + (eta * cosi - torch.sqrt(k.clamp_min(0.0))) * n
    return normalize(rd), valid


def _fresnel(cosi, eta):
    """Schlick reflectance approximation."""
    r0 = ((1.0 - eta) / (1.0 + eta)) ** 2
    return r0 + (1.0 - r0) * (1.0 - cosi).clamp(0.0, 1.0) ** 5


class SceneGPU:
    def __init__(self, scene: SceneData, device="cuda"):
        self.dev = device
        f = lambda a: torch.as_tensor(a, device=device, dtype=torch.float32)
        i = lambda a: torch.as_tensor(a, device=device, dtype=torch.long)

        self.V = f(scene.tri_v)
        self.Nv = f(scene.tri_n)
        self.tri_mat = i(scene.tri_mat)
        self.sph_c = f(scene.sph_c)
        self.sph_r = f(scene.sph_r)
        self.sph_mat = i(scene.sph_mat)
        self.q0 = f(scene.quad_q0)
        self.e1 = f(scene.quad_e1)
        self.e2 = f(scene.quad_e2)
        self.qn = f(scene.quad_n)
        self.quad_mat = i(scene.quad_mat)
        self.mtl = f(scene.mtl)
        self.light_pos = f(scene.light_pos)
        self.light_col = f(scene.light_col)
        self.ambient = f(scene.ambient)
        self.bg = f(scene.bg)

        self.bvh = BVHGpu.build(self.V, self.Nv)

        # opaque, non-emissive surfaces cast shadows
        diel = self.mtl[:, 13] > 0.5
        emis = self.mtl[:, 10:13].sum(-1) > 1e-4
        self.is_occluder = (~diel) & (~emis)
        self.quad_occ = self.is_occluder[self.quad_mat]
        self.sph_occ = self.is_occluder[self.sph_mat]

    def closest_hit(self, ro, rd):
        N = ro.shape[0]
        t = ro.new_full((N,), INF)
        n = ro.new_zeros((N, 3))
        mid = torch.full((N,), -1, dtype=torch.long, device=self.dev)

        tt, tn, ti = self.bvh.closest(ro, rd)
        better = tt < t
        t = torch.where(better, tt, t)
        n = torch.where(better[:, None], tn, n)
        mid = torch.where(better, self.tri_mat[ti.clamp_min(0)], mid)

        if self.sph_c.shape[0] > 0:
            st, sn, si = intersect_spheres(ro, rd, self.sph_c, self.sph_r)
            better = st < t
            t = torch.where(better, st, t)
            n = torch.where(better[:, None], sn, n)
            mid = torch.where(better, self.sph_mat[si], mid)

        if self.q0.shape[0] > 0:
            qt, qnv, qi = intersect_quads(ro, rd, self.q0, self.e1, self.e2, self.qn)
            better = qt < t
            t = torch.where(better, qt, t)
            n = torch.where(better[:, None], qnv, n)
            mid = torch.where(better, self.quad_mat[qi], mid)

        hit = t < INF
        return t, n, mid, hit

    def visibility(self, p, ns, tile=1_200_000):
        """Return [H,L] visibility (1=lit) of points p[H,3] to each light.

        Batches all shadow rays and tiles them to bound memory.
        """
        H = p.shape[0]
        L = self.light_pos.shape[0]
        origin = p + ns * (EPS * 4.0)
        lp = self.light_pos[None, :, :]
        vec = lp - origin[:, None, :]
        dist = vec.norm(dim=-1)
        ldir = vec / dist.clamp_min(1e-9)[..., None]

        ro = origin[:, None, :].expand(H, L, 3).reshape(-1, 3)
        rd = ldir.reshape(-1, 3)
        md = (dist - EPS * 8.0).reshape(-1)
        occ = torch.zeros(ro.shape[0], dtype=torch.bool, device=self.dev)
        for s in range(0, ro.shape[0], tile):
            e = min(s + tile, ro.shape[0])
            occ[s:e] = self._occluded(ro[s:e], rd[s:e], md[s:e])
        vis = (~occ).float().reshape(H, L)
        return vis, ldir, dist

    def _occluded(self, ro, rd, md):
        occ = self.bvh.occluded(ro, rd, md)
        if self.quad_occ.any():
            qt, _, qi = intersect_quads(ro, rd, self.q0, self.e1, self.e2, self.qn)
            occ = occ | ((qt < md) & self.quad_occ[qi])
        if self.sph_occ.any():
            st, _, si = intersect_spheres(ro, rd, self.sph_c, self.sph_r)
            occ = occ | ((st < md) & self.sph_occ[si])
        return occ

    def shade(self, p, n, view_dir, M):
        """Blinn-Phong direct lighting. view_dir points from hit toward eye."""
        albedo = M[:, 0:3]
        spec = M[:, 3:6]
        shin = M[:, 6:7]
        emis = M[:, 10:13]

        out = emis + self.ambient[None, :] * albedo
        vis, ldir, dist = self.visibility(p, n)
        falloff = 1.0 / dist.clamp_min(0.2) ** 2
        ndotl = (n[:, None, :] * ldir).sum(-1).clamp_min(0.0)
        halfv = normalize(ldir + view_dir[:, None, :])
        ndoth = (n[:, None, :] * halfv).sum(-1).clamp_min(0.0)
        spec_term = ndoth ** shin.clamp_min(1.0)  # shin is [H,1], broadcasts over L

        radiance = self.light_col[None, :, :] * (vis * falloff)[..., None]
        diff = albedo[:, None, :] * ndotl[..., None]
        sp = spec[:, None, :] * spec_term[..., None]
        out = out + (radiance * (diff + sp)).sum(dim=1)
        return out

    def trace(self, ro, rd, pix, P, max_depth):
        """Trace a flat ray set, accumulating radiance into [P,3]."""
        radiance = ro.new_zeros((P, 3))
        weight = ro.new_ones((ro.shape[0], 3))

        o, d, w, idx = ro, rd, weight, pix
        for level in range(max_depth + 1):
            if o.shape[0] == 0:
                break
            t, n, mid, hit = self.closest_hit(o, d)

            miss = ~hit
            if miss.any():
                radiance.index_add_(0, idx[miss], w[miss] * self.bg[None, :])

            if not bool(hit.any()):
                break

            o, d, w, idx = o[hit], d[hit], w[hit], idx[hit]
            t, n, mid = t[hit], n[hit], mid[hit]
            p = o + t[:, None] * d
            view_dir = -d
            # face the shading normal toward the incoming ray
            entering = (n * d).sum(-1) < 0.0
            ns = torch.where(entering[:, None], n, -n)

            M = self.mtl[mid]
            col = self.shade(p, ns, view_dir, M)
            radiance.index_add_(0, idx, w * col)

            if level == max_depth:
                break

            refl = M[:, 7]
            ior = M[:, 9]
            spec = M[:, 3:6]
            diel = M[:, 13] > 0.5

            children_o = []
            children_d = []
            children_w = []
            children_i = []

            rdir = normalize(d - 2.0 * (d * ns).sum(-1, keepdim=True) * ns)
            roff = p + ns * (EPS * 4.0)

            # dielectric: split into a reflected and a refracted ray via Fresnel
            if bool(diel.any()):
                eta = torch.where(entering, 1.0 / ior, ior)
                cosi = (-d * ns).sum(-1).clamp(0.0, 1.0)
                Fr = _fresnel(cosi, eta)
                tdir, tok = _refract(d, ns, eta)
                m = diel
                children_o.append(roff[m])
                children_d.append(rdir[m])
                children_w.append(w[m] * Fr[m, None])
                children_i.append(idx[m])
                # refracted ray where it exists
                mr = diel & tok
                toff = p - ns * (EPS * 4.0)
                children_o.append(toff[mr])
                children_d.append(tdir[mr])
                children_w.append(w[mr] * (1.0 - Fr[mr, None]))
                children_i.append(idx[mr])
                # total internal reflection folds transmitted energy into a reflection
                mt = diel & (~tok)
                if bool(mt.any()):
                    children_o.append(roff[mt])
                    children_d.append(rdir[mt])
                    children_w.append(w[mt] * (1.0 - Fr[mt, None]))
                    children_i.append(idx[mt])

            # metal/mirror: one reflection ray scaled by reflectivity
            mm = (~diel) & (refl > 1e-3)
            if bool(mm.any()):
                children_o.append(roff[mm])
                children_d.append(rdir[mm])
                children_w.append(w[mm] * (refl[mm, None] * spec[mm]))
                children_i.append(idx[mm])

            if not children_o:
                break
            o = torch.cat(children_o, 0)
            d = torch.cat(children_d, 0)
            w = torch.cat(children_w, 0)
            idx = torch.cat(children_i, 0)
            # cull rays whose contribution is negligible
            keep = w.max(dim=-1).values > 1e-3
            o, d, w, idx = o[keep], d[keep], w[keep], idx[keep]

        return radiance

    def render(self, cam: Camera, width, height, spp=1, max_depth=2, seed=0):
        P = width * height
        gen = torch.Generator(device=self.dev).manual_seed(seed)
        accum = torch.zeros((P, 3), device=self.dev)
        for s in range(spp):
            o, d = cam.rays(width, height, self.dev, jitter=(spp > 1), gen=gen)
            pix = torch.arange(P, device=self.dev)
            accum += self.trace(o, d, pix, P, max_depth)
        img = accum / spp
        return img.reshape(height, width, 3)
