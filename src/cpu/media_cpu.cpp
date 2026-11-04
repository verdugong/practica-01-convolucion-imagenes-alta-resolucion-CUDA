#include "practica.hpp"
#include <stdexcept>

namespace practica {
void mediaCPU(const Imagen& entrada, int k, std::vector<float>& salida)
{
    validarEntrada(entrada, k);
    if (&entrada.pixeles == &salida)
        throw std::invalid_argument("Entrada y salida deben ser distintas.");
    if (salida.size() != entrada.pixeles.size())
        throw std::invalid_argument("Reservar la salida antes de medir.");

    const int radio = k / 2;
    const float coeficiente = 1.0f / static_cast<float>(k * k);

    // Version secuencial: un solo hilo de CPU recorre todos los pixeles.
    for (int fila = 0; fila < entrada.alto; ++fila) {
        for (int columna = 0; columna < entrada.ancho; ++columna) {
            float acumulado = 0.0f;
            for (int dy = -radio; dy <= radio; ++dy) {
                for (int dx = -radio; dx <= radio; ++dx) {
                    const int y = fila + dy;
                    const int x = columna + dx;
                    // Fuera de la imagen: valor cero. NO renormalizar el borde.
                    if (y >= 0 && y < entrada.alto &&
                        x >= 0 && x < entrada.ancho) {
                        const std::size_t vecino =
                            static_cast<std::size_t>(y) * entrada.ancho + x;
                        acumulado += entrada.pixeles[vecino] * coeficiente;
                    }
                }
            }
            salida[static_cast<std::size_t>(fila) * entrada.ancho + columna] =
                acumulado;
        }
    }
}
} // namespace practica
