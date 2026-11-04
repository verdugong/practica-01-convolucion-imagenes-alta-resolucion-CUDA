// Realce: centro 5, vecinos -1; conserva negativos y valores >255.
#include "kernels.cuh"
#include <cuda_runtime.h>
#include <cstddef>
namespace practica {
namespace {
__global__ void nitidezKernel(const float* entrada, float* salida, int ancho, int alto) {
    // Indice lineal: bloques/hilos en X. Los hilos sobrantes no acceden a memoria.
    const std::size_t indice = static_cast<std::size_t>(blockIdx.x)*blockDim.x+threadIdx.x;
    if (indice >= static_cast<std::size_t>(ancho)*alto) return;
    // Division y residuo recuperan la coordenada bidimensional del pixel.
    const int fila = static_cast<int>(indice/ancho);
    const int columna = static_cast<int>(indice%ancho);
    const float mascara[9] = {0,-1,0,-1,5,-1,0,-1,0};
    float suma = 0.0f;
    // Vecinos fuera de la imagen equivalen a cero; no se renormaliza el borde.
    for (int dy=-1;dy<=1;++dy) {
        for (int dx=-1;dx<=1;++dx) {
            const int y=fila+dy, x=columna+dx;
            if (y>=0 && y<alto && x>=0 && x<ancho)
                suma += entrada[static_cast<std::size_t>(y)*ancho+x]*mascara[(dy+1)*3+dx+1];
        }
    }
    salida[indice]=suma;
}
}
void lanzarNitidez(const float* in, float* out, int w, int h, int bloques, int hilos) {
    nitidezKernel<<<bloques,hilos>>>(in,out,w,h);
}
}
