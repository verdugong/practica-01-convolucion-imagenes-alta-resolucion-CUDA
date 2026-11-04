#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace practica {
struct Imagen {
    int ancho = 0;
    int alto = 0;
    std::vector<float> pixeles; // intensidad 0..255; mismo tipo en CPU y GPU
};
struct Errores {
    double maximo = 0.0;
    double medio = 0.0;
    bool valido = false;
};
constexpr double TOLERANCIA = 0.001; // unidades de intensidad, ANTES de visualizar

void validarEntrada(const Imagen& imagen, int k);
Imagen cargarPGM(const std::string& ruta);
void guardarPGM(const std::string& ruta, int ancho, int alto,
                const std::vector<float>& valores);
Imagen crearDemo(); // imagen sintetica 17x19, solo para comprobar el programa
enum class Filtro { Media, Nitidez, Laplaciano };
Filtro leerFiltro(const std::string& nombre);
const char* nombreFiltro(Filtro filtro);
// Selecciona el filtro sin modificar la entrada; salida debe estar reservada.
void aplicarCPU(const Imagen&, Filtro, int k, std::vector<float>&);
void nitidezCPU(const Imagen&, std::vector<float>&);
void laplacianoCPU(const Imagen&, std::vector<float>&);
void guardarVisual(const std::string&, const Imagen&, const std::vector<float>&, Filtro);
void mediaCPU(const Imagen& entrada, int k, std::vector<float>& salida);
// No recorta ni convierte intensidades; NaN/inf siempre invalidan el resultado.
Errores comparar(const std::vector<float>& referencia,
                const std::vector<float>& resultado, double tolerancia = TOLERANCIA);
// Percentil con interpolacion lineal en la posicion p*(N-1).
double percentil(std::vector<double> valores, double p);
std::string descripcionCPUCompilador();
} // namespace practica
