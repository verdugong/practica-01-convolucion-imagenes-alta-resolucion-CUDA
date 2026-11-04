#include "practica.hpp"
#include "filtros_gpu.hpp"
#include <iostream>
#include <stdexcept>

using namespace practica;
int main()
{
    try {
        // 323 pixeles: no multiplo de 128, 256 ni 512.
        // Tambien se incluyen 1x1 y la matriz manual para verificar los bordes.
        for (const Imagen& imagen : {Imagen{1,1,{9}},
                Imagen{3,3,{1,2,3,4,5,6,7,8,9}}, crearDemo()}) {
            for (Filtro tipo : {Filtro::Media,Filtro::Nitidez,Filtro::Laplaciano}) {
                for (int k : {3,9,21}) {
                    if (tipo != Filtro::Media && k != 3) continue;
                    std::vector<float> cpu(imagen.pixeles.size());
                    std::vector<float> gpu(imagen.pixeles.size());
                    aplicarCPU(imagen, tipo, k, cpu);
                    for (int hilos : {128,256,512}) {
                        FiltroGPU filtro(imagen.ancho, imagen.alto, k, hilos, tipo);
                        filtro.ejecutar(imagen.pixeles, gpu);
                        const auto e = comparar(cpu, gpu);
                        std::cout << nombreFiltro(tipo) << ' ' << imagen.ancho << 'x' << imagen.alto << " k=" << k
                                  << " hilos=" << hilos << " max=" << e.maximo
                                  << " medio=" << e.medio << '\n';
                        if (!e.valido) throw std::runtime_error("CPU y GPU difieren.");
                    }
                }
            }
        }
        std::cout << "PRUEBAS CPU-GPU CORRECTAS (45 combinaciones).\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
