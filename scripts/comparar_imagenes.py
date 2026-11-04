"""Crear PNG comparativo completo y detalle al 100%, fuera de las mediciones."""

import argparse
from pathlib import Path
from PIL import Image, ImageDraw


def panel(imagenes, etiquetas, miniaturas):
    marg, espacio, titulo = 12, 12, 30
    if miniaturas:
        vistas = []
        for im in imagenes:
            copia = im.copy()
            copia.thumbnail((640, 640), Image.Resampling.LANCZOS)
            vistas.append(copia)
    else:
        vistas = imagenes
    ancho = max(im.width for im in vistas)
    alto = max(im.height for im in vistas)
    lienzo = Image.new(
        "RGB", (2 * marg + 3 * ancho + 2 * espacio, 2 * marg + titulo + alto), "white"
    )
    dibujo = ImageDraw.Draw(lienzo)
    for i, (im, etiqueta) in enumerate(zip(vistas, etiquetas)):
        x = marg + i * (ancho + espacio)
        dibujo.text((x, marg), etiqueta, fill="black")
        lienzo.paste(im, (x, marg + titulo))
    return lienzo


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("original", type=Path)
    p.add_argument("cpu", type=Path)
    p.add_argument("gpu", type=Path)
    p.add_argument("--out", type=Path, default=Path("results/comparacion"))
    p.add_argument("--x", type=int, default=None)
    p.add_argument("--y", type=int, default=None)
    p.add_argument("--lado", type=int, default=400)
    a = p.parse_args()
    imagenes = []
    for ruta in (a.original, a.cpu, a.gpu):
        with Image.open(ruta) as im:
            imagenes.append(im.convert("L"))
    if len({im.size for im in imagenes}) != 1:
        p.error("Las tres imagenes deben tener las mismas dimensiones.")
    ancho, alto = imagenes[0].size
    lado = min(a.lado, ancho, alto)
    if lado <= 0:
        p.error("--lado debe ser positivo.")
    x = (ancho - lado) // 2 if a.x is None else a.x
    y = (alto - lado) // 2 if a.y is None else a.y
    if x < 0 or y < 0 or x + lado > ancho or y + lado > alto:
        p.error("Recorte fuera de los limites.")
    rutas = [Path(str(a.out) + "_completa.png"), Path(str(a.out) + "_detalle.png")]
    if any(ruta.exists() for ruta in rutas):
        p.error("Ya existen salidas. Elegir otro prefijo --out.")
    a.out.parent.mkdir(parents=True, exist_ok=True)
    etiquetas = ["Original", "Salida CPU", "Salida GPU"]
    panel(imagenes, etiquetas, True).save(rutas[0])
    caja = (x, y, x + lado, y + lado)
    panel([im.crop(caja) for im in imagenes], etiquetas, False).save(rutas[1])
    print(f"Vista general: {rutas[0]}")
    print(f"Detalle sin reducir: {rutas[1]}; recorte {caja}")
    print("Estas vistas no sustituyen la validacion numerica float32.")


if __name__ == "__main__":
    main()
