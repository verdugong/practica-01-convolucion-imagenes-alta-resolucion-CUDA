#pragma once
#include "practica.hpp"
#include <memory>

namespace practica {
struct TiemposGPU {
    double h2d_ms = 0.0;
    double kernel_ms = 0.0;
    double d2h_ms = 0.0;
    double total_ms = 0.0;
};
// Reserva memoria y crea eventos una sola vez, fuera de las mediciones.
class FiltroGPU {
public:
    FiltroGPU(int ancho, int alto, int k, int hilos, Filtro filtro = Filtro::Media);
    ~FiltroGPU();
    FiltroGPU(const FiltroGPU&) = delete;
    FiltroGPU& operator=(const FiltroGPU&) = delete;
    TiemposGPU ejecutar(const std::vector<float>& entrada,
                        std::vector<float>& salida);
    std::string descripcion() const;
    int bloques() const;
private:
    struct Impl;
    std::unique_ptr<Impl> p_;
};
} // namespace practica
