# Practica 1: convolucion de imagenes con CPU y CUDA

Tres filtros en archivos separados, un ejecutable y cuatro scripts de apoyo.
Los scripts convierten imagenes y organizan resultados; la convolucion evaluada
se realiza exclusivamente en nuestro codigo C++/CUDA.

## 1. Estructura

| Ruta | Funcion |
|---|---|
| `CMakeLists.txt` | Compilacion C++17/CUDA y pruebas CTest. |
| `include/practica.hpp` | Imagen, filtros, errores y estadisticas. |
| `include/filtros_gpu.hpp` | Interfaz GPU sin exponer CUDA al main. |
| `include/kernels.cuh` | Declaraciones de los lanzadores de kernels. |
| `src/main.cpp` | Parametros, calentamiento, mediciones y CSV. |
| `src/cpu/media_cpu.cpp` | Media secuencial. |
| `src/cpu/nitidez_cpu.cpp` | Nitidez secuencial. |
| `src/cpu/laplaciano_cpu.cpp` | Laplaciano secuencial. |
| `src/gpu/media_gpu.cu` | Kernel de media. |
| `src/gpu/nitidez_gpu.cu` | Kernel de nitidez. |
| `src/gpu/laplaciano_gpu.cu` | Kernel de laplaciano. |
| `src/gpu/filtros_gpu.cu` | Memoria, transferencias y medicion CUDA compartidas. |
| `src/common/` | Lectura PGM, seleccion de filtro, visualizacion y validacion. |
| `tests/` | Referencia CPU y equivalencia CPU-GPU. |
| `scripts/convertir_entradas.py` | Convierte tus tres imagenes sin redimensionar. |
| `scripts/experimentos.py` | Ejecuta configuraciones y registra el equipo. |
| `scripts/graficar.py` | Tablas y graficas desde CSV. |
| `scripts/comparar_imagenes.py` | PNG completos y detalles al 100%. |
| `data/originales/` | Tus tres imagenes existentes. |
| `data/preparadas/` | Entradas PGM generadas. |
| `results/` | CSV, metadatos, imagenes y graficas generadas. |
| `build/` | Compilacion local; excluida de Git. |

## 2. Compilar y comprobar

Entorno previsto: Visual Studio 2022 C++ x64, CUDA Toolkit 12.9, CMake >=3.24,
RTX 3070 (arquitectura 86). Python >=3.10 para los scripts.
Extraer en una carpeta nueva. Desde la carpeta con `CMakeLists.txt`, en
**x64 Native Tools Command Prompt for VS 2022**:

```bat
cmake -S . -B build -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_ARCHITECTURES=86
cmake --build build
ctest --test-dir build --output-on-failure
py -m pip install -r requirements.txt
```

Se esperan dos pruebas aprobadas. Si `build` ya usa otro generador, cambiarlo
por `build_nmake` en todos los comandos. Si no existe `py`, usar `python`.

## 3. Tus imagenes existentes

Colocar las tres imagenes en `data/originales/`. Sustituir estos nombres por los
reales (PNG, JPEG, BMP, TIFF de 8 bits o PGM):

```bat
py scripts\convertir_entradas.py --img2000 "data\originales\foto_2000.png" --img4000 "data\originales\foto_4000.png" --img6000 "data\originales\foto_6000.png"
```

El script comprueba las dimensiones exactas y **no recorta ni redimensiona**.
Aplica orientacion EXIF y gris de 8 bits; si hay transparencia, compone sobre
blanco. Para RGB usa aproximadamente `0.299R + 0.587G + 0.114B`.
Genera `imagen_2000.pgm`, `imagen_4000.pgm`, `imagen_6000.pgm` y `entradas.json`
en `data/preparadas/`. PGM binario P5 evita dependencias externas de imagen en C++.

Completar la fuente, permiso y preparacion previa en `entradas.json`, o usar las
opciones `--origen "..." --preparacion-previa "..."`. El archivo registra hashes
SHA-256. Las tres imagenes deben tener el mismo contenido y encuadre: el script
solo comprueba dimensiones. Si ya son PGM P5 con maxval 255, puedes copiarlas
con esos nombres y documentar su procedencia en `data/README.md`.
No se sobrescriben entradas preparadas; usar otra carpeta `--out` para repetir.

## 4. Prueba rapida y experimentos

```bat
build\practica1.exe --demo --filter media --mask 3 --threads 256 --reps 5 --output results\demo_media
build\practica1.exe --demo --filter nitidez --mask 3 --threads 256 --reps 5 --output results\demo_nitidez
build\practica1.exe --demo --filter laplaciano --mask 3 --threads 256 --reps 5 --output results\demo_laplaciano
```

Cada comando debe mostrar `VALIDACION CPU-GPU CORRECTA`. La demo tiene 17x19
pixeles; sus tiempos no son los resultados de la practica. Cambiar `--output`
si se repite: el programa no sobrescribe resultados.

Ejecutar todos los experimentos con tus imagenes:

```bat
py scripts\experimentos.py --exe build\practica1.exe --data data\preparadas --reps 5
```

| Experimento | Variable | Condiciones fijas |
|---|---|---|
| Filtros | Media, nitidez, laplaciano | 4000x4000, mascara 3, 256 hilos |
| Resolucion | 2000, 4000, 6000 pixeles por lado | Media, mascara 9, 256 hilos |
| Mascara | 3, 9, 21 | Media, 4000x4000, 256 hilos |
| Hilos | 128, 256, 512 | Media, mascara 9, 4000x4000 |

Son nueve combinaciones distintas: se reutilizan las coincidentes, como permite
el enunciado. Todas se miden en CPU y GPU. La mascara 21 requiere mas trabajo de
CPU: esperar a que termine. Evitar otros trabajos intensivos mientras se mide.
Una validacion fallida detiene la corrida y conserva sus registros.
Cada ejecucion automatica crea una carpeta nueva con fecha/hora dentro de `results`.

Para un caso individual:

```bat
build\practica1.exe --input data\preparadas\imagen_4000.pgm --filter laplaciano --mask 3 --threads 256 --reps 5 --output results\laplaciano_individual
```

## 5. Salidas

- `*_raw.csv`: mediciones por repeticion, errores, configuracion y unidades.
- `*_summary.csv`: mediana, Q1, Q3 e IQR por metrica.
- `*_metadata.txt`, `*_consola.txt`: configuracion y registro de ejecucion.
- `*_cpu.pgm`, `*_gpu.pgm`: imagenes para visualizar.
- `experimentos.json`: plan, CPU, nucleos, RAM, herramientas, commit y hashes.
- `mediciones_completas.csv`, `resumen.csv`: datos reunidos, aceleraciones y errores.
- `figuras/`: seis graficas y comparativos completos/detalles de los tres filtros.

Las graficas se generan automaticamente. Para regenerarlas:

```bat
py scripts\graficar.py results\experimentos_AAAAMMDD_HHMMSS
```

Los comparativos usan un recorte central al 100%. Puedes elegir otra region con
`comparar_imagenes.py original.pgm cpu.pgm gpu.pgm --x 100 --y 200 --lado 400 --out results\detalle`.

## 6. Algoritmos y validacion

| Filtro | Coeficientes | Efecto |
|---|---|---|
| Media k x k | Todos `1/(k*k)` | Suaviza y reduce detalle. |
| Nitidez 3 x 3 | `[0,-1,0; -1,5,-1; 0,-1,0]` | Realza cambios locales. |
| Laplaciano 3 x 3 | `[0,1,0; 1,-4,1; 0,1,0]` | Detecta cambios de intensidad. |

Mascaras simetricas: correlacion y convolucion coinciden. CPU usa bucles
secuenciales, sin OpenMP. GPU usa bloques/hilos solo en X, un hilo por pixel,
lectura de imagen desde memoria global y arreglos separados de entrada/salida.
No hay memoria compartida, filtros separables ni bibliotecas de convolucion.

`indice = blockIdx.x*blockDim.x + threadIdx.x`; si `indice >= ancho*alto`, terminar.
`fila = indice/ancho`; `columna = indice%ancho`. La cantidad de bloques se calcula
con division redondeada hacia arriba. Fuera de la imagen hay ceros; la media
conserva el divisor `k*k` incluso en el borde. Cada filtro parte de la entrada original.

CPU/GPU usan `float`. Se validan antes de visualizar: error maximo
`max(abs(CPU-GPU))` y error medio `sum(abs(CPU-GPU))/pixeles`.
Tolerancia absoluta `0.001` unidades (aprox. 0.0004% de 255): muy inferior a
un nivel de gris. Es un umbral practico para diferencias de redondeo con el
mismo orden de acumulacion, no una cota universal frente a aritmetica exacta.
Se desactiva la fusion producto-suma en CPU y CUDA. No aumentar la tolerancia
para ocultar errores. Con entradas enteras, nitidez/laplaciano normalmente deben
coincidir exactamente. Se rechazan NaN e infinitos.

Prueba manual, entrada `[1,2,3;4,5,6;7,8,9]`:

- Media 3: `[12,21,16;27,45,33;24,39,28]/9`.
- Nitidez: `[-1,1,7;7,5,13;23,19,31]`.
- Laplaciano: `[2,1,-4;-3,0,-7;-16,-11,-22]`.

Tambien se prueban 1x1, un impulso de intensidad 255 y 17x19 (323 pixeles,
no multiplo de 128, 256 ni 512). La prueba GPU incluye 45 combinaciones.
Visualizacion: media/nitidez se saturan a [0,255] y redondean; laplaciano usa
`saturar(abs(L),0,255)`, con escala fija identica en CPU/GPU. Solo la imagen
visual pierde el signo y satura magnitudes >255. Las salidas float se validan
en memoria y sus errores quedan en CSV.

## 7. Metodologia de medicion

Un calentamiento CPU y GPU por configuracion; luego al menos cinco repeticiones.
Se alterna el orden CPU/GPU. Reservas, liberacion de memoria, archivos,
visualizacion y validacion quedan fuera del tiempo. Host usa memoria normal
`std::vector`, sin solapamiento de transferencias.

- CPU: `steady_clock` alrededor de la funcion secuencial.
- H2D/D2H: reloj host con finalizacion de cada transferencia.
- Kernel: eventos CUDA y sincronizacion del evento final.
- Total GPU: reloj host desde H2D hasta finalizar D2H; incluye lanzamiento,
  instrumentacion y sincronizaciones, por eso puede superar H2D+kernel+D2H.
- IQR = Q3-Q1, percentiles con interpolacion lineal.
- Aceleraciones = mediana CPU / mediana kernel y mediana CPU / mediana total GPU.
- Transferencias = mediana de `(H2D+D2H)/total GPU*100` por repeticion.

Las graficas muestran mediana y Q1-Q3; CPU/GPU tienen paneles con escalas distintas.
No se incluyen mediciones precargadas. Las conclusiones deben surgir de tu equipo.

## 8. GitHub y entrega

Versionar codigo, README, pruebas, CSV finales y recursos de graficas. `build`,
ejecutables y originales grandes se excluyen con `.gitignore`. Si no publicas
las entradas, completar `data/README.md` con enlace e instrucciones exactas.
Registrar `git rev-parse HEAD` y revisar el estado de Git antes de entregar.

El informe PDF y las diapositivas deben incluir tus resultados reales, equipo,
metodologia, validacion, figuras, respuestas a las cuatro preguntas del enunciado,
limitaciones, contribuciones, firmas, enlace del repositorio y commit.
Este paquete contiene el codigo para generar la evidencia, sin inventar resultados.

## 9. Verificacion y referencias

La parte CPU se compilo y probo en el entorno de preparacion. Los scripts se
comprobaron con datos de prueba. CUDA debe compilarse y ejecutarse en tu RTX 3070:
el entorno de preparacion no dispone de nvcc ni de GPU NVIDIA.

Para probar solo CPU en otro equipo (no sirve para los experimentos finales):

```sh
cmake -S . -B build_cpu -DPRACTICA_ENABLE_CUDA=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build_cpu
ctest --test-dir build_cpu --output-on-failure
```