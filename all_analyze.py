from analyze import *
import os

def analys_and_save_to_dir(file_path):
    """
    Загружает конкретный снапшот, строит графики,
    сохраняет рядом с ним (в ту же папку) как analysis.png.
    Окно не открывается.
    """
    if not os.path.exists(file_path):
        print(f"[skip] File not found: {file_path}")
        return

    directory = os.path.dirname(file_path)
    base_name = os.path.splitext(os.path.basename(file_path))[0]
    save_path = os.path.join(directory, f"{base_name}_analysis.png")

    try:
        snap = load_snapshot(file_path)

        mass        = snap["mass"]
        pos         = snap["pos"]
        vel         = snap["vel"]
        dark_flag   = snap["dark_matter"]
        movable     = snap["movable"]

        baryon_mask = (dark_flag == 0)
        center = find_center_spherical(pos[baryon_mask], mass[baryon_mask])
        v_center = velocity_at_center(pos[baryon_mask], vel[baryon_mask], mass[baryon_mask], center)

        pos_rel = pos - center
        vel_rel = vel - v_center

        movable_mask = (movable == 1)
        if not movable_mask.any():
            raise ValueError("Нет подвижных частиц")

        L_vec = compute_angular_momentum(pos_rel, vel_rel, mass, movable_mask)
        L_norm = np.linalg.norm(L_vec)
        if L_norm < 1e-12:
            print(f"[warning] L ≈ 0 in {file_path}")
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
            save_path=save_path,
        )
        print(f"[ok] Saved: {save_path}")

    except Exception as e:
        print(f"[error] {file_path}: {e}")

def main():

    DIR = "bin/saves/test_simulations/"

    filenames = sorted(f for f in os.listdir(DIR))
    for i, filename in enumerate(filenames, 1):
        FILE_PATH = os.path.join(DIR, filename)
        analys_and_save_to_dir(FILE_PATH)
    
if __name__ == "__main__":
    main()