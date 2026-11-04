"""Ejecutar todas las configuraciones obligatorias, sin repetir combinaciones identicas."""

import argparse
import csv
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import psutil
from PIL import Image
from convertir_entradas import sha256


def configuraciones():
    """Plan unico: las combinaciones compartidas se miden una sola vez."""
    return [
        ("media", 4000, 3, 256, ["filtros", "mascaras"]),
        ("nitidez", 4000, 3, 256, ["filtros"]),
        ("laplaciano", 4000, 3, 256, ["filtros"]),
        ("media", 2000, 9, 256, ["resoluciones"]),
        ("media", 4000, 9, 256, ["resoluciones", "mascaras", "hilos"]),
        ("media", 6000, 9, 256, ["resoluciones"]),
        ("media", 4000, 21, 256, ["mascaras"]),
        ("media", 4000, 9, 128, ["hilos"]),
        ("media", 4000, 9, 512, ["hilos"]),
    ]


def capturar(command):
    """Registrar herramientas disponibles sin perder el resto de los metadatos."""
    try:
        result = subprocess.run(
            command, capture_output=True, text=True, errors="replace", timeout=30
        )
        return dict(
            command=command,
            codigo=result.returncode,
            stdout=result.stdout,
            stderr=result.stderr,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        return dict(command=command, error=str(exc))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument(
        "--exe",
        type=Path,
        default=Path("build/practica1.exe" if os.name == "nt" else "build/practica1"),
    )
    p.add_argument("--data", type=Path, default=Path("data/preparadas"))
    p.add_argument("--out", type=Path, default=None)
    p.add_argument("--reps", type=int, default=5)
    args = p.parse_args()
    if not 5 <= args.reps <= 1000:
        p.error("--reps debe estar entre 5 y 1000.")
    if not args.exe.is_file():
        p.error(f"No existe {args.exe}; compilar primero.")
    inputs = {}
    for n in (2000, 4000, 6000):
        file = args.data / f"imagen_{n}.pgm"
        with file.open("rb") as stream:
            magic = stream.read(2)
        with Image.open(file) as im:
            if im.size != (n, n) or im.mode != "L" or magic != b"P5":
                p.error(f"{file}: se requiere PGM P5 de 8 bits, {n}x{n}.")
        inputs[str(n)] = dict(path=str(file.resolve()), sha256=sha256(file))
    output = args.out or Path("results") / datetime.now().strftime(
        "experimentos_%Y%m%d_%H%M%S"
    )
    if output.exists():
        p.error("La carpeta de resultados ya existe; elegir otra --out.")
    # Evitar una corrida obligatoria CPU-only por error de configuracion.
    probe = subprocess.run(
        [str(args.exe.resolve()), "--cuda-disponible"], capture_output=True, text=True
    )
    if probe.returncode != 0:
        p.error(
            "El ejecutable no tiene CUDA habilitado. Compilar con PRACTICA_ENABLE_CUDA=ON."
        )
    output.mkdir(parents=True)
    plan = []
    for f, n, k, t, groups in configuraciones():
        plan.append(
            dict(
                filtro=f,
                resolucion=n,
                mascara=k,
                hilos=t,
                experimentos=groups,
                prefijo=f"{f}_{n}_k{k}_t{t}",
            )
        )
    manifest = dict(
        inicio_utc=datetime.now(timezone.utc).isoformat(),
        repeticiones=args.reps,
        entradas=inputs,
        configuraciones=plan,
        estado="en curso",
        sistema=platform.platform(),
        cpu=platform.processor() or os.getenv("PROCESSOR_IDENTIFIER"),
        nucleos_fisicos=psutil.cpu_count(logical=False),
        nucleos_logicos=psutil.cpu_count(),
        ram_bytes=psutil.virtual_memory().total,
        python=sys.version,
        herramientas=[
            capturar(c)
            for c in [
                ["nvcc", "--version"],
                ["cmake", "--version"],
                ["nvidia-smi"],
                ["git", "rev-parse", "HEAD"],
                ["git", "status", "--short"],
                [sys.executable, "-m", "pip", "freeze"],
            ]
        ],
    )
    manifest_path = output / "experimentos.json"

    def save():
        manifest_path.write_text(
            json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
        )

    save()
    try:
        for index, c in enumerate(plan, 1):
            prefix = output / c["prefijo"]
            command = [
                str(args.exe.resolve()),
                "--input",
                str(args.data / f"imagen_{c['resolucion']}.pgm"),
                "--filter",
                c["filtro"],
                "--mask",
                str(c["mascara"]),
                "--threads",
                str(c["hilos"]),
                "--reps",
                str(args.reps),
                "--output",
                str(prefix),
            ]
            print(f"\n[{index}/{len(plan)}] {c['prefijo']}", flush=True)
            # Heredar consola: progreso visible; log de cada corrida para el informe.
            with Path(str(prefix) + "_consola.txt").open("w", encoding="utf-8") as log:
                proc = subprocess.Popen(
                    command,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    errors="replace",
                    bufsize=1,
                )
                for line in proc.stdout:
                    print(line, end="", flush=True)
                    log.write(line)
                if proc.wait():
                    raise RuntimeError(f"Fallo {c['prefijo']}; revisar consola y CSV.")
            with Path(str(prefix) + "_raw.csv").open(newline="") as f:
                gpu = [r for r in csv.DictReader(f) if r["implementacion"] == "CUDA"]
            if len(gpu) != args.reps or any(r["validado"] != "true" for r in gpu):
                raise RuntimeError("Falta validacion CUDA completa; corrida rechazada.")
            c["completado"] = True
            save()
        manifest["estado"] = "mediciones completas"
        save()
        subprocess.run(
            [sys.executable, str(Path(__file__).with_name("graficar.py")), str(output)],
            check=True,
        )
        # Comparativos de los tres filtros, siempre desde la misma entrada.
        for c in plan[:3]:
            prefix = output / c["prefijo"]
            subprocess.run(
                [
                    sys.executable,
                    str(Path(__file__).with_name("comparar_imagenes.py")),
                    str(args.data / "imagen_4000.pgm"),
                    str(prefix) + "_cpu.pgm",
                    str(prefix) + "_gpu.pgm",
                    "--out",
                    str(output / "figuras" / c["filtro"]),
                ],
                check=True,
            )
        manifest["estado"] = "completo"
        save()
    except Exception as exc:
        manifest["estado"] = "interrumpido"
        manifest["error"] = str(exc)
        save()
        raise
    print(f"\nTerminado: {output}. Revisar resumen.csv, figuras y entradas.json.")


if __name__ == "__main__":
    main()
