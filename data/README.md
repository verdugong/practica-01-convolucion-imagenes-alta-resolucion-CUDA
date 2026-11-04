# Datos de entrada

## Fuente y autor

La imagen utilizada muestra a un chef colocando una pizza en un horno de
ladrillo. Fue descargada desde Pexels:

- Autor: **Nano Erdozain**
- Fuente: [Chef colocando pizza en horno de ladrillo para hornear](https://www.pexels.com/es-es/foto/chef-colocando-pizza-en-horno-de-ladrillo-para-hornear-29021744/)
- Plataforma: Pexels
- Acceso: la imagen aparece disponible para descarga gratuita en la página
  consultada el 4 de noviembre de 2026.

La descarga gratuita no sustituye la revisión de las condiciones de uso de
Pexels. Para esta práctica se utiliza la imagen únicamente como entrada de
procesamiento y comparación de filtros, conservando la atribución al autor y
la referencia a la fuente original.

## Preparación de las entradas

Se utilizó la misma imagen y el mismo encuadre para generar tres resoluciones
cuadradas:

- `imagen_2000.pgm`: 2000 x 2000 píxeles.
- `imagen_4000.pgm`: 4000 x 4000 píxeles.
- `imagen_6000.pgm`: 6000 x 6000 píxeles.

Las imágenes originales se descargaron desde Pexels y se prepararon previamente
con el mismo contenido y recorte cuadrado. Después se ejecutó
`scripts/convertir_entradas.py`, que:

1. Verifica que cada archivo tenga exactamente la resolución esperada.
2. Corrige la orientación EXIF cuando existe.
3. Convierte la imagen a escala de grises de 8 bits.
4. Compone sobre fondo blanco las imágenes con transparencia.
5. Guarda cada entrada como PGM binario P5 con valores de 0 a 255.
6. Calcula y registra hashes SHA-256 de los archivos originales y preparados.

El conversor no vuelve a redimensionar ni recortar las imágenes; únicamente
convierte el formato y la representación de color. La conversión a escala de
grises utiliza la luminancia aproximada
`0.299R + 0.587G + 0.114B`.

Para repetir el proceso:

```bat
py scripts\convertir_entradas.py ^
  --img2000 "data\originales\foto_2000.jpg" ^
  --img4000 "data\originales\foto_4000.jpg" ^
  --img6000 "data\originales\foto_6000.jpg" ^
  --origen "Nano Erdozain, Pexels: https://www.pexels.com/es-es/foto/chef-colocando-pizza-en-horno-de-ladrillo-para-hornear-29021744/" ^
  --preparacion-previa "Misma imagen y encuadre cuadrado preparados en 2000x2000, 4000x4000 y 6000x6000; sin redimensionamiento adicional durante la conversión a PGM."
```

El archivo `preparadas/entradas.json` registra la conversión, las dimensiones,
la versión de Pillow y los hashes SHA-256.
