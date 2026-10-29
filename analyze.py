"""
Анализ галактического снимка: центр, ось вращения, профиль плотности,
кривая вращения.

Данные: CSV, первая строка — заголовок с world_time, вторая — шапка таблицы,
далее N строк по 10 колонок:
    id, mass, px, py, pz, vx, vy, vz, dark_matter, movable
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D


# ============================================================================
# КОНФИГУРАЦИЯ
# ============================================================================

FILE_PATH = "bin/saves/test_simulations/galaxy_2026-10-29_16-23-59-077.csv"

# --- Параметры поиска центра ---
CENTER_SHRINK       = 0.6
CENTER_MIN_FACTOR   = 0.02
CENTER_MAX_ITER     = 80
CENTER_TOL          = 1e-6

# --- Параметры сферического профиля плотности ---
DENSITY_BINS        = 50
DENSITY_RMIN_PCT    = 1.0
DENSITY_RMAX_PCT    = 98.0

# --- Параметры кривой вращения ---
ROTATION_RMAX_PCT   = 95

# --- Эталонные профили ТМ ---
EINASTO_ALPHA       = 0.17


# ============================================================================
# ФИЗИЧЕСКИЕ ЕДИНИЦЫ
# ============================================================================

UNITS_ENABLED = True

# --- Код-единицы ---
L_SCALE_KPC  = 3.0        # кпк на 1 код-ед. длины
M_SCALE_MSUN = 1.0e7      # M☉ на 1 код-ед. массы
T_SCALE_MYR  = 774.5      # млн лет на 1 код-ед. времени

# --- Коэффициенты для подписи графиков ---
R_FACTOR   = L_SCALE_KPC                          # → кпк
M_FACTOR   = M_SCALE_MSUN                         # → M☉
T_FACTOR   = T_SCALE_MYR                          # → млн лет
RHO_FACTOR = M_SCALE_MSUN / L_SCALE_KPC**3        # → M☉/кпк³
V_FACTOR   = L_SCALE_KPC / T_SCALE_MYR            # → кпк/млн лет


# ============================================================================
# ЗАГРУЗКА ДАННЫХ
# ============================================================================

def load_snapshot(path):
    with open(path, 'r', encoding='utf-8') as f:
        world_time = float(f.readline().split(',')[1])

    data = np.loadtxt(path, delimiter=',', skiprows=2)
    if data.ndim != 2 or data.shape[1] < 10:
        raise ValueError(f"Ожидалось ≥10 колонок, получено {data.shape}")
    if data.shape[0] < 10:
        raise ValueError("Слишком мало частиц для анализа")

    return {
        "world_time":  world_time,
        "mass":        data[:, 1],
        "pos":         data[:, 2:5],
        "vel":         data[:, 5:8],
        "dark_matter": data[:, 8].astype(np.int8),
        "movable":     data[:, 9].astype(np.int8),
    }


# ============================================================================
# ЦЕНТР СИСТЕМЫ
# ============================================================================

def find_center_spherical(pos, mass, r_start=None,
                          shrink=CENTER_SHRINK,
                          r_min_factor=CENTER_MIN_FACTOR,
                          n_iter=CENTER_MAX_ITER,
                          tol=CENTER_TOL):
    c = (pos * mass[:, None]).sum(0) / mass.sum()

    if r_start is None:
        r_start = np.percentile(np.linalg.norm(pos - c, axis=1), 50)

    R = r_start
    r_min = r_min_factor * r_start

    for _ in range(n_iter):
        d = np.linalg.norm(pos - c, axis=1)
        m = d < R
        if m.sum() < 10:
            break
        w = mass[m]
        c_new = (pos[m] * w[:, None]).sum(0) / w.sum()
        shift = np.linalg.norm(c_new - c)
        c = c_new
        if shift < tol:
            break
        R = max(R * shrink, r_min)

    return c


def velocity_at_center(pos, vel, mass, center, window_pct=30):
    d = np.linalg.norm(pos - center, axis=1)
    r_win = np.percentile(d, window_pct)
    mask = d < r_win
    return (vel[mask] * mass[mask, None]).sum(0) / mass[mask].sum()


# ============================================================================
# ОСЬ ВРАЩЕНИЯ И ПОВОРОТ
# ============================================================================

def compute_angular_momentum(pos, vel, mass, mask):
    p = pos[mask]
    v = vel[mask]
    w = mass[mask]
    return np.sum(w[:, None] * np.cross(p, v), axis=0)


def rotation_matrix_from_to(a, b):
    a = a / np.linalg.norm(a)
    b = b / np.linalg.norm(b)
    v = np.cross(a, b)
    c = float(np.dot(a, b))
    s = float(np.linalg.norm(v))

    if s < 1e-10:
        if c > 0:
            return np.eye(3)
        helper = np.array([1.0, 0.0, 0.0]) if abs(a[0]) < 0.9 else np.array([0.0, 1.0, 0.0])
        axis = np.cross(a, helper)
        n = np.linalg.norm(axis)
        if n < 1e-12:
            helper = np.array([0.0, 0.0, 1.0])
            axis = np.cross(a, helper)
            n = np.linalg.norm(axis)
        axis /= n
        K = np.array([[0, -axis[2], axis[1]],
                      [axis[2], 0, -axis[0]],
                      [-axis[1], axis[0], 0]])
        return np.eye(3) + 2.0 * (K @ K)

    K = np.array([[0, -v[2], v[1]],
                  [v[2], 0, -v[0]],
                  [-v[1], v[0], 0]])
    return np.eye(3) + K + (K @ K) * ((1.0 - c) / (s ** 2))


# ============================================================================
# ПРОФИЛЬ СФЕРИЧЕСКОЙ ПЛОТНОСТИ
# ============================================================================

def spherical_density_profile(radii, mass, masks,
                              n_bins=DENSITY_BINS,
                              rmin_pct=DENSITY_RMIN_PCT,
                              rmax_pct=DENSITY_RMAX_PCT):
    r_min = np.percentile(radii, rmin_pct)
    r_max = np.percentile(radii, rmax_pct)

    edges = np.geomspace(r_min, r_max, n_bins + 1)
    centers = np.sqrt(edges[:-1] * edges[1:])
    vol_shell = (4.0 / 3.0) * np.pi * (edges[1:]**3 - edges[:-1]**3)

    densities = {}
    errors = {}
    counts = {}

    for name, m in masks.items():
        m_f = m.astype(float)
        w = mass * m_f

        m_sel, _ = np.histogram(radii, bins=edges, weights=w)
        densities[name] = m_sel / vol_shell

        w2_sel, _ = np.histogram(radii, bins=edges, weights=w**2)
        with np.errstate(divide='ignore', invalid='ignore'):
            N_eff = np.where(w2_sel > 0, m_sel**2 / w2_sel, 0.0)
        rel_err = np.where(N_eff > 0, 1.0 / np.sqrt(N_eff), np.nan)
        err = densities[name] * rel_err
        errors[name] = np.where(np.isfinite(err), err, 0.0)

        c_sel, _ = np.histogram(radii, bins=edges, weights=m_f)
        counts[name] = c_sel

    return centers, edges, densities, errors, counts


def cumulative_density_profile(radii, mass, masks, n_bins=DENSITY_BINS,
                               rmin_pct=DENSITY_RMIN_PCT,
                               rmax_pct=DENSITY_RMAX_PCT):
    r_min = np.percentile(radii, rmin_pct)
    r_max = np.percentile(radii, rmax_pct)
    r_grid = np.geomspace(r_min, r_max, n_bins)

    order = np.argsort(radii)
    r_sorted = radii[order]

    densities = {}
    errors = {}

    for name, m in masks.items():
        m_f = m.astype(float)
        w = (mass * m_f)[order]
        w2 = (mass * m_f)[order] ** 2

        M_cum  = np.cumsum(w)
        W2_cum = np.cumsum(w2)

        M_at_r  = np.interp(r_grid, r_sorted, M_cum)
        W2_at_r = np.interp(r_grid, r_sorted, W2_cum)

        V = (4.0 / 3.0) * np.pi * r_grid**3
        densities[name] = M_at_r / V

        with np.errstate(divide='ignore', invalid='ignore'):
            N_eff = np.where(W2_at_r > 0, M_at_r**2 / W2_at_r, 0.0)
        rel_err = np.where(N_eff > 0, 1.0 / np.sqrt(N_eff), np.nan)
        err = densities[name] * rel_err
        errors[name] = np.where(np.isfinite(err), err, 0.0)

    return r_grid, densities, errors


# ============================================================================
# ЭТАЛОННЫЕ ПРОФИЛИ ТЁМНОЙ МАТЕРИИ
# ============================================================================

def nfw_profile(r, rho_s, r_s):
    x = r / r_s
    return rho_s / (x * (1.0 + x) ** 2)


def einasto_profile(r, rho_s, r_s, alpha=EINASTO_ALPHA):
    return rho_s * np.exp(-(2.0 / alpha) * ((r / r_s) ** alpha - 1.0))

# не надо
def isothermal_profile(r, rho_0, r_0):
    return rho_0 / (1.0 + (r / r_0) ** 2)


def make_reference_profiles(r_grid, rho_data, r_ref_idx):
    r0 = r_grid[r_ref_idx]
    rho0 = rho_data[r_ref_idx]

    nfw = nfw_profile(r_grid, rho_s=4.0 * rho0, r_s=r0)
    ein = einasto_profile(r_grid, rho_s=rho0, r_s=r0, alpha=EINASTO_ALPHA)
    iso = isothermal_profile(r_grid, rho_0=2.0 * rho0, r_0=r0)

    return {"nfw": nfw, "einasto": ein, "isothermal": iso}


# ============================================================================
# КРИВАЯ ВРАЩЕНИЯ
# ============================================================================

def tangential_velocity(pos, vel):
    R = np.hypot(pos[:, 0], pos[:, 1])
    R_safe = np.where(R == 0, np.nan, R)
    return R, (pos[:, 0] * vel[:, 1] - pos[:, 1] * vel[:, 0]) / R_safe


# ============================================================================
# ИНТЕРАКТИВНАЯ ЛЕГЕНДА
# ============================================================================

def _flatten_artists(obj):
    if hasattr(obj, "lines"):
        yield from obj.lines
        if hasattr(obj, "caps"):
            yield from obj.caps
        if hasattr(obj, "barlinecols"):
            yield from obj.barlinecols
    else:
        yield obj


def make_legend_interactive(ax, fig, entries, loc="upper right", fontsize=9):
    proxies = [e["proxy"] for e in entries]
    labels  = [e["label"] for e in entries]

    leg = ax.legend(proxies, labels, fontsize=fontsize, loc=loc)
    leg.set_draggable(True)

    for p in proxies:
        p.set_picker(8)
    for t in leg.get_texts():
        t.set_picker(8)

    proxy_to_idx = {id(p): i for i, p in enumerate(proxies)}
    text_to_idx  = {id(t): i for i, t in enumerate(leg.get_texts())}
    state = [True] * len(entries)

    def _apply(idx):
        for art in entries[idx]["artists"]:
            for a in _flatten_artists(art):
                a.set_visible(state[idx])
        leg.get_texts()[idx].set_alpha(1.0 if state[idx] else 0.30)

    def on_pick(event):
        art = event.artist
        idx = proxy_to_idx.get(id(art))
        if idx is None:
            idx = text_to_idx.get(id(art))
        if idx is None:
            return
        state[idx] = not state[idx]
        _apply(idx)
        fig.canvas.draw_idle()

    fig.canvas.mpl_connect('pick_event', on_pick)
    return leg


# ============================================================================
# ВИЗУАЛИЗАЦИЯ
# ============================================================================

def plot_results(spherical_centers,
                 densities, errors, counts,
                 cum_r_grid, cum_densities,
                 radii, v_theta, dark_mask, baryon_mask,
                 r_max_display, world_time,
                 min_count=3,
                 save_path=None):
    # ------------------------------------------------------------------
    # Единицы: применяем ТОЛЬКО здесь, при выводе
    # ------------------------------------------------------------------
    if UNITS_ENABLED:
        r_unit   = "kpc"
        rho_unit = r"$M_\odot/\mathrm{kpc}^3$"
        v_unit   = "kpc / Myr"
        t_unit   = "Myr"
        rf, rhof, vf = R_FACTOR, RHO_FACTOR, V_FACTOR
        t_disp = world_time * T_FACTOR
    else:
        r_unit   = r"$L_0$"
        rho_unit = r"$M_0/L_0^3$"
        v_unit   = r"$L_0/t_0$"
        t_unit   = r"$t_0$"
        rf = rhof = vf = 1.0
        t_disp = world_time

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6.5))

    # ==================================================================
    # ГРАФИК 1: сферическая плотность
    # ==================================================================
    rho_dark   = densities["dark"]
    rho_baryon = densities["baryon"]
    err_dark   = errors["dark"]

    def _mask(x, y, c=None, mc=min_count):
        y = np.asarray(y, dtype=float)
        ok = (y > 0) & np.isfinite(y)
        if c is not None:
            ok &= (np.asarray(c) >= mc)
        return np.asarray(x), np.where(ok, y, np.nan)

    # --- Дифференциальная плотность оболочки ---
    x, y = _mask(spherical_centers, rho_dark, counts["dark"])
    line_dark_shell, = ax1.loglog(
        x * rf, y * rhof, color='teal', lw=2.2, ls='-', zorder=4)

    x, y = _mask(spherical_centers, rho_baryon, counts["baryon"])
    line_baryon_shell, = ax1.loglog(
        x * rf, y * rhof, color='crimson', lw=2.2, ls='-', zorder=4)

    # --- Кумулятивная плотность ---
    x, y = _mask(cum_r_grid, cum_densities["dark"])
    line_dark_cum, = ax1.loglog(
        x * rf, y * rhof, color='teal', lw=1.0, alpha=0.55, ls='-', zorder=3)

    x, y = _mask(cum_r_grid, cum_densities["baryon"])
    line_baryon_cum, = ax1.loglog(
        x * rf, y * rhof, color='crimson', lw=1.0, alpha=0.55, ls='-', zorder=3)


    # --- Эталоны ---
    ref_lines = {}
    valid = (rho_dark > 0) & np.isfinite(rho_dark) & (counts["dark"] >= min_count)
    if valid.any():
        idx_valid = np.where(valid)[0]
        r_ref_idx = idx_valid[len(idx_valid) // 2]
        refs = make_reference_profiles(spherical_centers, rho_dark, r_ref_idx)

        ref_lines["nfw"], = ax1.loglog(
            spherical_centers * rf, refs["nfw"] * rhof,
            color='gray', ls='--', lw=1.4, alpha=0.85, zorder=2)
        ref_lines["einasto"], = ax1.loglog(
            spherical_centers * rf, refs["einasto"] * rhof,
            color='gray', ls=':', lw=1.4, alpha=0.85, zorder=2)
        ref_lines["isothermal"], = ax1.loglog(
            spherical_centers * rf, refs["isothermal"] * rhof,
            color='gray', ls=(0, (5, 1, 1, 1)), lw=1.4, alpha=0.85, zorder=2)

    ax1.set_title('Spherical density profile', fontsize=12, pad=10)
    ax1.set_xlabel(f"Radius [{r_unit}]", fontsize=11)
    ax1.set_ylabel(rf"$\rho(R)$ [{rho_unit}]", fontsize=11)
    ax1.grid(True, which='both', linestyle=':', alpha=0.5)

    # --- Легенда ax1 ---
    entries1 = [
        {"label": "Dark matter (shell)",
         "artists": [line_dark_shell],
         "proxy": Line2D([], [], color='teal', lw=2.2)},
        {"label": "Baryon matter (shell)",
         "artists": [line_baryon_shell],
         "proxy": Line2D([], [], color='crimson', lw=2.2)},
        {"label": "Dark matter (cumulative)",
         "artists": [line_dark_cum],
         "proxy": Line2D([], [], color='teal', lw=1.0, alpha=0.55)},
        {"label": "Baryon matter (cumulative)",
         "artists": [line_baryon_cum],
         "proxy": Line2D([], [], color='crimson', lw=1.0, alpha=0.55)},
    ]
    if ref_lines:
        entries1 += [
            {"label": "NFW (reference)",
             "artists": [ref_lines["nfw"]],
             "proxy": Line2D([], [], color='gray', ls='--', lw=1.4)},
            {"label": "Einasto (reference)",
             "artists": [ref_lines["einasto"]],
             "proxy": Line2D([], [], color='gray', ls=':', lw=1.4)},
            {"label": "Isothermal (reference)",
             "artists": [ref_lines["isothermal"]],
             "proxy": Line2D([], [], color='gray', ls=(0, (5, 1, 1, 1)), lw=1.4)},
        ]

    make_legend_interactive(ax1, fig, entries1, loc='lower left', fontsize=8.5)

    # ==================================================================
    # ГРАФИК 2: кривая вращения
    # ==================================================================
    ax2.set_xlim(0, r_max_display * rf)

    sc_dark = ax2.scatter(
        radii[dark_mask] * rf, v_theta[dark_mask] * vf,
        color='teal', s=2, alpha=0.30)
    sc_baryon = ax2.scatter(
        radii[baryon_mask] * rf, v_theta[baryon_mask] * vf,
        color='crimson', s=2, alpha=0.55)

    ax2.axhline(0, color='black', linestyle=':', alpha=0.4)
    ax2.set_title('Orbital velocity distribution', fontsize=12, pad=10)
    ax2.set_xlabel(f"Radius [{r_unit}]", fontsize=11)
    ax2.set_ylabel(f"Orbital velocity [{v_unit}]", fontsize=11)
    ax2.grid(True, linestyle='--', alpha=0.5)

    entries2 = [
        {"label": "Dark matter",
         "artists": [sc_dark],
         "proxy": Line2D([], [], color='teal', marker='o', ls='', ms=8)},
        {"label": "Baryon matter",
         "artists": [sc_baryon],
         "proxy": Line2D([], [], color='crimson', marker='o', ls='', ms=8)},
    ]

    make_legend_interactive(ax2, fig, entries2, loc='upper right', fontsize=10)

    # ==================================================================
    # Заголовок
    # ==================================================================
    fig.suptitle(f"Galaxy snapshot  |  world_time = {t_disp:.3f} {t_unit}",
                 fontsize=12, y=1.00)
    plt.tight_layout()

    if save_path:
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        plt.close(fig)  # <-- закрыть фигуру, не открывать окно
    else:
        plt.show()  # <-- режим для интерактива


# ============================================================================
# MAIN
# ============================================================================

def main():
    snap = load_snapshot(FILE_PATH)

    mass        = snap["mass"]
    pos         = snap["pos"]
    vel         = snap["vel"]
    dark_flag   = snap["dark_matter"]
    movable     = snap["movable"]

    baryon_mask = (dark_flag == 0)
    center = find_center_spherical(pos[baryon_mask], mass[baryon_mask]) # тока по барионам
    v_center = velocity_at_center(pos[baryon_mask], vel[baryon_mask], mass[baryon_mask], center)

    pos_rel = pos - center
    vel_rel = vel - v_center

    movable_mask = (movable == 1)
    if not movable_mask.any():
        raise ValueError("Нет подвижных частиц — ось вращения неопределена")

    L_vec = compute_angular_momentum(pos_rel, vel_rel, mass, movable_mask)
    L_norm = np.linalg.norm(L_vec)
    if L_norm < 1e-12:
        print("[warning] L ≈ 0 — система не вращается, ось Z взята по умолчанию")
        normal_vec = np.array([0.0, 0.0, 1.0])
    else:
        normal_vec = L_vec / L_norm

    R = rotation_matrix_from_to(normal_vec, np.array([0.0, 0.0, 1.0]))
    pos_rotated = pos_rel @ R.T
    vel_rotated = vel_rel @ R.T

    dark_mask   = (dark_flag == 1)
    baryon_mask = (dark_flag == 0)

    radii_3d = np.linalg.norm(pos_rel, axis=1)
    sph_centers, _, densities, errors, counts = spherical_density_profile(
        radii_3d, mass,
        masks={"dark": dark_mask, "baryon": baryon_mask},
    )

    cum_r, cum_densities, _ = cumulative_density_profile(
        radii_3d, mass,
        masks={"dark": dark_mask, "baryon": baryon_mask},
    )

    radii, v_theta = tangential_velocity(pos_rotated, vel_rotated)
    r_max_display = np.percentile(radii, ROTATION_RMAX_PCT)

    plot_results(
        spherical_centers=sph_centers,
        densities=densities,
        errors=errors,
        counts=counts,
        cum_r_grid=cum_r,
        cum_densities=cum_densities,
        radii=radii,
        v_theta=v_theta,
        dark_mask=dark_mask,
        baryon_mask=baryon_mask,
        r_max_display=r_max_display,
        world_time=snap["world_time"],
    )


if __name__ == "__main__":
    main()