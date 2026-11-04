#include "practica.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace practica {
namespace {
std::string token(std::istream& archivo)
{
    std::string resultado;
    char c;
    while (archivo.get(c)) {
        if (c == '#') {
            archivo.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        } else if (!std::isspace(static_cast<unsigned char>(c))) {
            resultado += c;
            break;
        }
    }
    while (archivo.get(c)) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            // Consumir CRLF como un solo separador. No saltar bytes del raster.
            if (c == '\r' && archivo.peek() == '\n') archivo.get();
            break;
        }
        resultado += c;
    }
    if (resultado.empty()) throw std::runtime_error("Cabecera PGM incompleta.");
    return resultado;
}
int enteroPositivo(const std::string& valor)
{
    std::size_t usados = 0;
    const long numero = std::stol(valor, &usados);
    if (usados != valor.size() || numero <= 0 || numero > 100000)
        throw std::runtime_error("Numero invalido en cabecera PGM.");
    return static_cast<int>(numero);
}
}
Imagen cargarPGM(const std::string& ruta)
{
    std::ifstream archivo(ruta, std::ios::binary);
    if (!archivo) throw std::runtime_error("No se pudo abrir: " + ruta);
    if (token(archivo) != "P5")
        throw std::runtime_error("Se requiere PGM binario P5; usar convertir_entradas.py.");
    Imagen imagen;
    imagen.ancho = enteroPositivo(token(archivo));
    imagen.alto = enteroPositivo(token(archivo));
    if (enteroPositivo(token(archivo)) != 255)
        throw std::runtime_error("PGM debe tener valor maximo 255.");
    const std::size_t cantidad =
        static_cast<std::size_t>(imagen.ancho) * imagen.alto;
    if (cantidad > 100000000)
        throw std::runtime_error("Limite de esta practica: 100 millones de pixeles.");
    std::vector<unsigned char> bytes(cantidad);
    archivo.read(reinterpret_cast<char*>(bytes.data()),
                 static_cast<std::streamsize>(cantidad));
    if (archivo.gcount() != static_cast<std::streamsize>(cantidad))
        throw std::runtime_error("Archivo PGM truncado.");
    imagen.pixeles.assign(bytes.begin(), bytes.end());
    return imagen;
}
void guardarPGM(const std::string& ruta, int ancho, int alto,
                const std::vector<float>& valores)
{
    if (ancho <= 0 || alto <= 0 ||
        valores.size() != static_cast<std::size_t>(ancho) * alto)
        throw std::invalid_argument("Salida de imagen invalida.");
    std::vector<unsigned char> bytes(valores.size());
    for (std::size_t i = 0; i < valores.size(); ++i) {
        if (!std::isfinite(valores[i]))
            throw std::runtime_error("No se pueden visualizar valores no finitos.");
        // Solo para visualizar: saturar a [0,255] y redondear al entero cercano.
        bytes[i] = static_cast<unsigned char>(
            std::lround(std::clamp(valores[i], 0.0f, 255.0f)));
    }
    std::ofstream archivo(ruta, std::ios::binary);
    if (!archivo) throw std::runtime_error("No se pudo crear: " + ruta);
    archivo << "P5\n" << ancho << ' ' << alto << "\n255\n";
    archivo.write(reinterpret_cast<const char*>(bytes.data()),
                  static_cast<std::streamsize>(bytes.size()));
    if (!archivo) throw std::runtime_error("Error al guardar: " + ruta);
}
Imagen crearDemo()
{
    Imagen imagen{17, 19, std::vector<float>(17 * 19)};
    for (int y = 0; y < imagen.alto; ++y)
        for (int x = 0; x < imagen.ancho; ++x)
            imagen.pixeles[y * imagen.ancho + x] =
                static_cast<float>((x * 17 + y * 29) % 256);
    return imagen;
}
} // namespace practica
