#include "practica.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace practica {
void validarEntrada(const Imagen& imagen, int k)
{
    if (imagen.ancho <= 0 || imagen.alto <= 0 ||
        imagen.pixeles.size() != static_cast<std::size_t>(imagen.ancho) * imagen.alto)
        throw std::invalid_argument("Dimensiones o arreglo de imagen incorrectos.");
    if (k != 3 && k != 9 && k != 21)
        throw std::invalid_argument("Mascara permitida: 3, 9 o 21.");
}
Errores comparar(const std::vector<float>& referencia,
                const std::vector<float>& resultado, double tolerancia)
{
    if (referencia.empty() || referencia.size() != resultado.size() ||
        !std::isfinite(tolerancia) || tolerancia < 0)
        throw std::invalid_argument("Parametros invalidos para la comparacion.");
    Errores errores;
    double suma = 0;
    for (std::size_t i = 0; i < referencia.size(); ++i) {
        if (!std::isfinite(referencia[i]) || !std::isfinite(resultado[i])) {
            errores.maximo = errores.medio = std::numeric_limits<double>::infinity();
            return errores; // valido permanece false
        }
        const double diferencia =
            std::abs(static_cast<double>(referencia[i]) - resultado[i]);
        errores.maximo = std::max(errores.maximo, diferencia);
        suma += diferencia;
    }
    errores.medio = suma / static_cast<double>(referencia.size());
    errores.valido = errores.maximo <= tolerancia;
    return errores;
}
double percentil(std::vector<double> valores, double p)
{
    if (valores.empty() || !std::isfinite(p) || p < 0 || p > 1)
        throw std::invalid_argument("Percentil invalido.");
    std::sort(valores.begin(), valores.end());
    const double posicion = p * (valores.size() - 1);
    const auto inferior = static_cast<std::size_t>(std::floor(posicion));
    const auto superior = static_cast<std::size_t>(std::ceil(posicion));
    return valores[inferior] +
           (posicion - inferior) * (valores[superior] - valores[inferior]);
}
std::string descripcionCPUCompilador()
{
#ifdef _MSC_VER
    return "MSVC " + std::to_string(_MSC_VER);
#elif defined(__GNUC__)
    return std::string("GCC ") + __VERSION__;
#else
    return "Compilador C++ no identificado";
#endif
}
} // namespace practica
