// Segunda derivada discreta: centro -4 y vecinos +1; conserva el signo.
#include "practica.hpp"
#include <stdexcept>
namespace practica {
void laplacianoCPU(const Imagen& entrada, std::vector<float>& salida)
{
    validarEntrada(entrada, 3);
    if (&entrada.pixeles == &salida || salida.size() != entrada.pixeles.size())
        throw std::invalid_argument("Reservar entrada y salida separadas.");
    constexpr float mascara[9] = {0,1,0,1,-4,1,0,1,0};
    // Vecinos fuera de la imagen equivalen a cero; no se renormaliza el borde.
    for (int fila = 0; fila < entrada.alto; ++fila) {
        for (int columna = 0; columna < entrada.ancho; ++columna) {
            float suma = 0.0f;
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    const int y = fila + dy, x = columna + dx;
                    if (y >= 0 && y < entrada.alto && x >= 0 && x < entrada.ancho)
                        suma += entrada.pixeles[static_cast<std::size_t>(y)*entrada.ancho+x]
                              * mascara[(dy+1)*3+dx+1];
                }
            }
            salida[static_cast<std::size_t>(fila)*entrada.ancho+columna] = suma;
        }
    }
}
} // namespace practica
