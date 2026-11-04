#include "practica.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace practica;
void exigir(bool condicion, const char* mensaje)
{
    if (!condicion) throw std::runtime_error(mensaje);
}
int main()
{
    try {
        // Calculo manual con ceros fuera de la matriz:
        // entrada  [1 2 3; 4 5 6; 7 8 9]
        // sumas 3x3 [12 21 16; 27 45 33; 24 39 28], todas divididas para 9.
        Imagen a{3, 3, {1,2,3,4,5,6,7,8,9}};
        std::vector<float> salida(9);
        mediaCPU(a, 3, salida);
        const std::vector<float> esperado =
            {12.f/9,21.f/9,16.f/9,27.f/9,45.f/9,33.f/9,24.f/9,39.f/9,28.f/9};
        exigir(comparar(esperado, salida, 0.00001).valido, "Fallo matriz manual.");
        nitidezCPU(a, salida);
        exigir(comparar({-1,1,7,7,5,13,23,19,31},salida,0).valido,
               "Fallo nitidez matriz manual.");
        laplacianoCPU(a, salida);
        exigir(comparar({2,1,-4,-3,0,-7,-16,-11,-22},salida,0).valido,
               "Fallo laplaciano matriz manual.");
        Imagen impulso{3,3,{0,0,0,0,255,0,0,0,0}};
        nitidezCPU(impulso,salida);
        exigir(salida[4]==1275 && salida[1]==-255, "Nitidez se recorto antes de validar.");
        laplacianoCPU(impulso,salida);
        exigir(salida[4]==-1020 && salida[1]==255, "Laplaciano perdio signo.");
        for (int k : {3,9,21}) {
            // Un pixel aislado vale 9/(k*k), NO 9: borde no renormalizado.
            Imagen uno{1,1,{9}};
            std::vector<float> s(1);
            mediaCPU(uno, k, s);
            exigir(std::abs(s[0] - 9.f/(k*k)) < 0.00001f, "Fallo borde 1x1.");
        }
        auto demo = crearDemo();
        salida.resize(demo.pixeles.size());
        mediaCPU(demo, 21, salida);
        for (float valor : salida)
            exigir(std::isfinite(valor) && valor >= 0 && valor <= 255,
                    "Media fuera de rango.");
        exigir(!comparar({1}, {2}).valido, "No detecta diferencia numerica.");
        exigir(!comparar({1}, {std::numeric_limits<float>::quiet_NaN()}).valido,
               "No detecta NaN.");
        const auto e = comparar({0,10}, {0,12}, 2);
        exigir(e.maximo == 2 && e.medio == 1 && e.valido, "Metricas incorrectas.");
        exigir(percentil({1,2,3,4,5}, .5) == 3 &&
               percentil({1,2,3,4,5}, .75) - percentil({1,2,3,4,5}, .25) == 2,
               "Mediana o IQR incorrecto.");

        // El primer byte de raster es salto de linea: no debe perderse.
        const auto ruta = std::filesystem::temp_directory_path() /
                          "practica1_prueba_cpu_io.pgm";
        guardarPGM(ruta.string(), 2, 2, {10,32,13,35});
        const auto vuelta = cargarPGM(ruta.string());
        exigir(vuelta.ancho == 2 && vuelta.alto == 2 &&
               comparar({10,32,13,35}, vuelta.pixeles, 0).valido, "Fallo PGM binario.");
        {
            std::ofstream f(ruta, std::ios::binary);
            f << "P5\r\n# comentario\r\n2 2\r\n255\r\n";
            const unsigned char bytes[] = {10,32,13,35};
            f.write(reinterpret_cast<const char*>(bytes), 4);
        }
        exigir(comparar({10,32,13,35}, cargarPGM(ruta.string()).pixeles, 0).valido,
               "Fallo cabecera CRLF.");
        std::filesystem::remove(ruta);
        std::cout << "PRUEBAS CPU CORRECTAS: matriz manual, bordes, validacion, "
                     "estadistica y PGM.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
