"""Generar tablas y graficas desde mediciones reales; nunca inventa datos faltantes."""

import argparse
import csv
import json
import math
from pathlib import Path
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("resultados", type=Path)
    args = p.parse_args()
    root = args.resultados
    manifest = json.loads((root / "experimentos.json").read_text(encoding="utf-8"))
    rows = []
    all_raw = []
    for config in manifest["configuraciones"]:
        prefix = root / config["prefijo"]
        with Path(str(prefix) + "_summary.csv").open(newline="") as f:
            stats = {
                r["metrica"]: {
                    key: float(r[key]) for key in ("mediana", "q1", "q3", "iqr")
                }
                for r in csv.DictReader(f)
            }
        with Path(str(prefix) + "_raw.csv").open(newline="") as f:
            raw = list(csv.DictReader(f))
            all_raw.extend(raw)
        gpu = [r for r in raw if r["implementacion"] == "CUDA"]
        cpu = [r for r in raw if r["implementacion"] == "CPU"]
        if (
            len(gpu) < 5
            or len(cpu) != len(gpu)
            or any(r["validado"] != "true" for r in gpu)
        ):
            raise ValueError(f"{prefix}: faltan datos CPU/CUDA validos.")
        for metric in ("cpu", "kernel", "h2d", "d2h", "total_gpu"):
            if metric not in stats or not all(
                math.isfinite(v) for v in stats[metric].values()
            ):
                raise ValueError(f"{prefix}: tiempos ausentes o no finitos.")
        if stats["kernel"]["mediana"] <= 0 or stats["total_gpu"]["mediana"] <= 0:
            raise ValueError("Mediana CUDA nula: no se puede calcular aceleracion.")
        record = {
            k: config[k]
            for k in ("filtro", "resolucion", "mascara", "hilos", "prefijo")
        }
        record["experimentos"] = ";".join(config["experimentos"])
        for metric, values in stats.items():
            for name, value in values.items():
                record[f"{metric}_{name}"] = value
        record["speedup_computo"] = stats["cpu"]["mediana"] / stats["kernel"]["mediana"]
        record["speedup_total"] = (
            stats["cpu"]["mediana"] / stats["total_gpu"]["mediana"]
        )
        record["error_max"] = max(float(r["error_max"]) for r in gpu)
        record["error_medio_peor"] = max(float(r["error_medio"]) for r in gpu)
        rows.append(record)
    for name, data in [("resumen.csv", rows), ("mediciones_completas.csv", all_raw)]:
        with (root / name).open("w", newline="", encoding="utf-8") as f:
            w = csv.DictWriter(f, fieldnames=list(data[0]))
            w.writeheader()
            w.writerows(data)
    figures = root / "figuras"
    figures.mkdir(exist_ok=True)
    plt.rcParams.update(
        {"font.size": 10, "axes.spines.top": False, "axes.spines.right": False}
    )
    for group, key, title in [
        ("filtros", "filtro", "Filtros: 4000 x 4000, mascara 3"),
        ("resoluciones", "resolucion", "Resolucion: media 9 x 9"),
        ("mascaras", "mascara", "Mascara: media, 4000 x 4000"),
        ("hilos", "hilos", "Hilos: media 9 x 9, 4000 x 4000"),
    ]:
        subset = [r for r in rows if group in r["experimentos"].split(";")]
        if key != "filtro":
            subset.sort(key=lambda r: r[key])
        labels = [str(r[key]) for r in subset]
        fig, axes = plt.subplots(1, 2, figsize=(12, 4.6))
        for ax, metrics in zip(axes, [["cpu"], ["kernel", "total_gpu"]]):
            for metric in metrics:
                y = [r[f"{metric}_mediana"] for r in subset]
                low = [
                    max(0, r[f"{metric}_mediana"] - r[f"{metric}_q1"]) for r in subset
                ]
                high = [
                    max(0, r[f"{metric}_q3"] - r[f"{metric}_mediana"]) for r in subset
                ]
                ax.errorbar(
                    range(len(labels)),
                    y,
                    yerr=[low, high],
                    marker="o",
                    linestyle="none" if group == "filtros" else "-",
                    capsize=5,
                    label=metric,
                )
            ax.set_xticks(range(len(labels)), labels)
            ax.set_ylabel("Tiempo (ms)")
            ax.set_xlabel(key)
            ax.grid(axis="y", alpha=0.25)
            ax.legend()
            ax.set_ylim(bottom=0)
        fig.suptitle(title + " | mediana y Q1-Q3")
        fig.tight_layout()
        fig.savefig(figures / f"{group}_tiempos.png", dpi=180)
        plt.close(fig)
    labels = [
        f"{r['filtro']}\n{r['resolucion']} / k{r['mascara']} / t{r['hilos']}"
        for r in rows
    ]
    fig, ax = plt.subplots(figsize=(13, 5))
    x = list(range(len(rows)))
    width = 0.38
    ax.bar(
        [i - width / 2 for i in x],
        [r["speedup_computo"] for r in rows],
        width,
        label="Computo CPU/kernel",
    )
    ax.bar(
        [i + width / 2 for i in x],
        [r["speedup_total"] for r in rows],
        width,
        label="CPU/total GPU",
    )
    ax.axhline(1, color="gray", linestyle="--")
    ax.set_ylabel("Aceleracion (x)")
    ax.set_xticks(x, labels, rotation=35, ha="right")
    ax.legend()
    fig.tight_layout()
    fig.savefig(figures / "aceleracion.png", dpi=180)
    plt.close(fig)
    fig, ax = plt.subplots(figsize=(13, 5))
    ax.bar(x, [r["proporcion_transferencias_mediana"] for r in rows], color="#287e8d")
    ax.set_xticks(x, labels, rotation=35, ha="right")
    ax.set_ylim(0, 100)
    ax.set_ylabel("Mediana de (H2D + D2H) / total GPU (%)")
    fig.tight_layout()
    fig.savefig(figures / "transferencias.png", dpi=180)
    plt.close(fig)
    print(f"Tablas y seis graficas guardadas en {root}")


if __name__ == "__main__":
    main()
