"""Convertir las tres imagenes existentes a PGM sin cambiar sus dimensiones."""

import argparse
import hashlib
import json
from pathlib import Path
from PIL import Image, ImageOps, __version__ as pillow_version


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as f:
        for block in iter(lambda: f.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for size in (2000, 4000, 6000):
        parser.add_argument(f"--img{size}", type=Path, required=True)
    parser.add_argument("--out", type=Path, default=Path("data/preparadas"))
    parser.add_argument(
        "--origen", default="PENDIENTE: indicar autor, fuente y permiso de uso"
    )
    parser.add_argument(
        "--preparacion-previa",
        default="PENDIENTE: documentar recorte comun y metodo de redimensionamiento ya realizado",
    )
    args = parser.parse_args()
    targets = [args.out / f"imagen_{n}.pgm" for n in (2000, 4000, 6000)]
    manifest = args.out / "entradas.json"
    if any(p.exists() for p in targets + [manifest]):
        parser.error("Ya existen archivos de salida. Use otra carpeta --out.")
    # Verificar las tres antes de escribir. Rechazar modos ambiguos de 16 bits/HDR.
    for n in (2000, 4000, 6000):
        path = getattr(args, f"img{n}")
        with Image.open(path) as im:
            if im.size != (n, n):
                parser.error(
                    f"{path}: tiene {im.size}, se requiere {n}x{n}. No se redimensionara."
                )
            if im.mode not in ("1", "L", "LA", "P", "RGB", "RGBA", "CMYK"):
                parser.error(
                    f"{path}: modo {im.mode} no admitido. Exportar a PNG RGB/L de 8 bits."
                )
            im.verify()
    args.out.mkdir(parents=True, exist_ok=True)
    records = []
    for n, target in zip((2000, 4000, 6000), targets):
        source = getattr(args, f"img{n}")
        with Image.open(source) as im:
            im = ImageOps.exif_transpose(im)
            if im.mode in ("RGBA", "LA", "P"):
                rgba = im.convert("RGBA")
                white = Image.new("RGBA", rgba.size, (255, 255, 255, 255))
                im = Image.alpha_composite(white, rgba).convert("RGB")
            im.convert("L").save(target)
        records.append(
            dict(
                dimensiones=[n, n],
                original=str(source.resolve()),
                sha256_original=sha256(source),
                pgm=target.name,
                sha256_pgm=sha256(target),
            )
        )
        print(f"{source} -> {target} ({n}x{n}, sin redimensionar)")
    metadata = dict(
        origen=args.origen,
        preparacion_previa=args.preparacion_previa,
        conversion="Orientacion EXIF; transparencia sobre blanco; Pillow convert L (luma RGB 0.299R+0.587G+0.114B), 8 bits; PGM P5 maxval 255. Sin recorte ni redimensionamiento.",
        pillow=pillow_version,
        entradas=records,
    )
    manifest.write_text(
        json.dumps(metadata, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print("Documentar que las tres imagenes tienen el mismo contenido y encuadre.")


if __name__ == "__main__":
    main()
