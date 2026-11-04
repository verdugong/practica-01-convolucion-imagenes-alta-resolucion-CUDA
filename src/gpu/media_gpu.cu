#include "kernels.cuh"
#include <cuda_runtime.h>
#include <cstddef>
namespace practica {
namespace {
// Un hilo procesa un pixel leyendo directamente de memoria global.
__global__ void mediaKernel(const float* entrada, float* salida,
                            int ancho, int alto, int k, float coeficiente)
{
    const std::size_t indice =
        static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (indice >= static_cast<std::size_t>(ancho) * alto) return;
    const int fila = static_cast<int>(indice / ancho);
    const int columna = static_cast<int>(indice % ancho);
    const int radio = k / 2;
    float acumulado = 0.0f;
    for (int dy = -radio; dy <= radio; ++dy) {
        for (int dx = -radio; dx <= radio; ++dx) {
            const int y = fila + dy;
            const int x = columna + dx;
            if (y >= 0 && y < alto && x >= 0 && x < ancho) {
                acumulado += entrada[static_cast<std::size_t>(y) * ancho + x]
                           * coeficiente;
            }
        }
    }
    salida[indice] = acumulado;
}
}
void lanzarMedia(const float* in, float* out, int w, int h, int k, int bloques, int hilos) {
    mediaKernel<<<bloques,hilos>>>(in,out,w,h,k,1.0f/static_cast<float>(k*k));
}
}
