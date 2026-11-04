#include "practica.hpp"
#include <cmath>
#include <stdexcept>
namespace practica {
Filtro leerFiltro(const std::string& nombre) {
    if (nombre == "media") return Filtro::Media;
    if (nombre == "nitidez") return Filtro::Nitidez;
    if (nombre == "laplaciano") return Filtro::Laplaciano;
    throw std::invalid_argument("Filtro permitido: media, nitidez o laplaciano.");
}
const char* nombreFiltro(Filtro f) {
    switch(f) {
        case Filtro::Media: return "media";
        case Filtro::Nitidez: return "nitidez";
        case Filtro::Laplaciano: return "laplaciano";
    }
    throw std::invalid_argument("Filtro invalido.");
}
void aplicarCPU(const Imagen& im, Filtro f, int k, std::vector<float>& out) {
    if (f != Filtro::Media && k != 3)
        throw std::invalid_argument("Nitidez y laplaciano requieren mascara 3.");
    switch(f) {
        case Filtro::Media: mediaCPU(im,k,out); break;
        case Filtro::Nitidez: nitidezCPU(im,out); break;
        case Filtro::Laplaciano: laplacianoCPU(im,out); break;
        default: throw std::invalid_argument("Filtro invalido.");
    }
}
void guardarVisual(const std::string& ruta, const Imagen& im,
                   const std::vector<float>& valores, Filtro f) {
    if (f != Filtro::Laplaciano) {
        guardarPGM(ruta,im.ancho,im.alto,valores); return;
    }
    // Visualizacion de magnitud con escala fija. No altera datos de validacion.
    // saturar(abs(L),0,255): elimina signo y satura magnitudes >255.
    std::vector<float> magnitud(valores.size());
    for (std::size_t i=0;i<valores.size();++i) magnitud[i]=std::abs(valores[i]);
    guardarPGM(ruta,im.ancho,im.alto,magnitud);
}
}
