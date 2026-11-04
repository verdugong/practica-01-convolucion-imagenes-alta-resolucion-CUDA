// Flujo: entrada -> reservas -> calentamiento -> medir/validar -> CSV e imagenes.
#include "practica.hpp"
#ifdef PRACTICA_WITH_CUDA
#include "filtros_gpu.hpp"
#endif
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

namespace fs = std::filesystem;
using namespace practica;
using Reloj = std::chrono::steady_clock;

struct Opciones {
    Filtro filtro = Filtro::Media;
    std::string entrada;
    std::string prefijo = "results/ensayo";
    int k = 3, hilos = 256, repeticiones = 5;
    bool demo = false;
};
void ayuda()
{
    std::cout
      << "Uso: practica1 (--input imagen.pgm | --demo) [opciones]\n"
      << "  --filter media|nitidez|laplaciano (predeterminado media)\n"
      << "  --mask 3|9|21       Tamano de la mascara de media (predeterminado 3)\n"
      << "  --threads 128|256|512  Hilos por bloque (predeterminado 256)\n"
      << "  --reps N           Repeticiones medidas, minimo 5\n"
      << "  --output prefijo   Prefijo NUEVO de archivos (results/ensayo)\n"
      << "  --help             Mostrar esta ayuda\n"
      << "Tres filtros CPU/GPU. --demo NO sustituye las fotos obligatorias.\n";
}
int leerEntero(const std::string& texto)
{
    std::size_t usados = 0;
    const int numero = std::stoi(texto, &usados);
    if (usados != texto.size()) throw std::invalid_argument("Entero invalido: " + texto);
    return numero;
}
Opciones opciones(int argc, char** argv)
{
    Opciones o;
    for (int i = 1; i < argc; ++i) {
        const std::string clave = argv[i];
        if (clave == "--demo") { o.demo = true; continue; }
        if (clave != "--input" && clave != "--output" && clave != "--mask" &&
            clave != "--threads" && clave != "--reps" && clave != "--filter")
            throw std::invalid_argument("Opcion desconocida: " + clave);
        if (++i >= argc) throw std::invalid_argument("Falta valor de " + clave);
        const std::string valor = argv[i];
        if (clave == "--input") o.entrada = valor;
        else if (clave == "--filter") o.filtro = leerFiltro(valor);
        else if (clave == "--output") o.prefijo = valor;
        else if (clave == "--mask") o.k = leerEntero(valor);
        else if (clave == "--threads") o.hilos = leerEntero(valor);
        else if (clave == "--reps") o.repeticiones = leerEntero(valor);
    }
    if (o.demo == !o.entrada.empty())
        throw std::invalid_argument("Elegir exactamente una entrada: --input o --demo.");
    if ((o.k != 3 && o.k != 9 && o.k != 21) ||
        (o.hilos != 128 && o.hilos != 256 && o.hilos != 512) ||
        o.repeticiones < 5 || o.repeticiones > 1000 || o.prefijo.empty())
        throw std::invalid_argument("Parametros invalidos. Consultar --help.");
    if (o.filtro != Filtro::Media && o.k != 3)
        throw std::invalid_argument("Nitidez y laplaciano requieren --mask 3.");
    return o;
}
std::ofstream abrir(const std::string& nombre)
{
    std::ofstream f(nombre);
    if (!f) throw std::runtime_error("No se pudo crear " + nombre);
    f << std::setprecision(12);
    return f;
}
void registrarResumen(std::ofstream& f, const char* nombre,
                      const char* unidad, const std::vector<double>& valores)
{
    const double mediana = percentil(valores, 0.5);
    const double q1 = percentil(valores, 0.25), q3 = percentil(valores, 0.75);
    f << nombre << ',' << unidad << ',' << mediana << ',' << q1
      << ',' << q3 << ',' << q3 - q1 << '\n';
    std::cout << nombre << ": mediana=" << mediana
              << ' ' << unidad << ", IQR=" << q3 - q1 << '\n';
}
int main(int argc, char** argv)
{
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help")) {
            ayuda(); return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--cuda-disponible") {
#ifdef PRACTICA_WITH_CUDA
            return 0;
#else
            return 2;
#endif
        }
        std::cout << std::unitbuf;
        const Opciones o = opciones(argc, argv);
        const Imagen imagen = o.demo ? crearDemo() : cargarPGM(o.entrada);
        validarEntrada(imagen, o.k);
        for (const char* sufijo : {"_raw.csv", "_summary.csv", "_cpu.pgm",
                                   "_gpu.pgm", "_metadata.txt"})
            if (fs::exists(o.prefijo + sufijo))
                throw std::runtime_error("El prefijo ya existe. Elegir otro --output.");
        const fs::path padre = fs::path(o.prefijo).parent_path();
        if (!padre.empty()) fs::create_directories(padre);
        std::vector<float> salidaCPU(imagen.pixeles.size());

#ifdef PRACTICA_WITH_CUDA
        std::vector<float> salidaGPU(imagen.pixeles.size());
        FiltroGPU gpu(imagen.ancho, imagen.alto, o.k, o.hilos, o.filtro);
        std::cout << gpu.descripcion();
#else
        std::cout << "MODO SOLO CPU: esta compilacion NO ejecuta ni valida CUDA.\n";
#endif
        std::cout << "Filtro: " << nombreFiltro(o.filtro) << ' ' << o.k << 'x' << o.k
                  << "\nImagen: " << imagen.ancho << 'x' << imagen.alto << '\n';
        if (o.demo) std::cout << "Datos sinteticos de prueba; NO son resultados de la practica.\n";
#ifndef NDEBUG
        std::cout << "AVISO: compilacion sin NDEBUG. Usar Release para tiempos finales.\n";
#endif
        auto metadatos = abrir(o.prefijo + "_metadata.txt");
        const auto ahora = std::time(nullptr);
        metadatos << "Inicio UTC: " << std::put_time(std::gmtime(&ahora), "%Y-%m-%dT%H:%M:%SZ")
                  << "\nEntrada: " << (o.demo ? "demo sintetica 17x19" : fs::absolute(o.entrada).string())
                  << "\nAncho: " << imagen.ancho << "\nAlto: " << imagen.alto
                  << "\nFiltro: " << nombreFiltro(o.filtro) << "\nMascara: " << o.k
                  << "\nRepeticiones: " << o.repeticiones
                  << "\nCalentamiento: 1 ejecucion CPU y 1 GPU (si esta habilitada)"
                  << "\nCompilador CPU: " << descripcionCPUCompilador()
                  << "\nProcesadores logicos detectados: " << std::thread::hardware_concurrency()
                  << "\nTipo numerico: float32, intensidades 0..255"
                  << "\nBordes: ceros; media con denominador k*k fijo"
                  << "\nTolerancia absoluta: " << TOLERANCIA
                  << "\nMemoria host: pageable; reservas previas excluidas"
                  << "\nValidacion y E/S de archivos: excluidas del tiempo"
                  << "\nCPU: steady_clock; kernel: eventos CUDA"
                  << "\nTransferencias: steady_clock + finalizacion"
                  << "\nTotal GPU: reloj host, incluye instrumentacion y sincronizacion"
                  << "\nVisualizacion: media/nitidez saturacion [0,255]; laplaciano saturar(abs(L),0,255); redondeo. Misma escala CPU/GPU"
                  << "\nOrden CPU/GPU: alternado entre repeticiones\n";
        if (const char* cpu = std::getenv("PROCESSOR_IDENTIFIER"))
            metadatos << "CPU: " << cpu << '\n';
#ifdef PRACTICA_WITH_CUDA
        metadatos << gpu.descripcion();
#else
        metadatos << "CUDA no compilado. Sin validacion CPU-GPU.\n";
#endif
        // Calentamiento y validacion inicial, fuera del cronometro.
        aplicarCPU(imagen, o.filtro, o.k, salidaCPU);
#ifdef PRACTICA_WITH_CUDA
        gpu.ejecutar(imagen.pixeles, salidaGPU);
        if (!comparar(salidaCPU, salidaGPU).valido)
            throw std::runtime_error("Fallo de validacion durante calentamiento.");
#endif
        auto csv = abrir(o.prefijo + "_raw.csv");
        csv << "implementacion,filtro,ancho,alto,mascara,hilos,bloques,repeticion,"
               "cpu_ms,h2d_ms,kernel_ms,d2h_ms,total_ms,error_max,error_medio,"
               "tolerancia,validado,unidad\n";
        std::vector<double> cpu, h2d, kernel, d2h, total, transferencias;
        double peorError = 0.0;
        for (int r = 1; r <= o.repeticiones; ++r) {
            double tiempoCPU = 0;
            const auto medirCPU = [&]() {
                const auto inicio = Reloj::now();
                aplicarCPU(imagen, o.filtro, o.k, salidaCPU);
                tiempoCPU = std::chrono::duration<double, std::milli>(
                    Reloj::now() - inicio).count();
            };
#ifdef PRACTICA_WITH_CUDA
            TiemposGPU t;
            if (r % 2 == 1) {
                medirCPU(); t = gpu.ejecutar(imagen.pixeles, salidaGPU);
            } else {
                t = gpu.ejecutar(imagen.pixeles, salidaGPU); medirCPU();
            }
            // Comparar float32 ANTES de redondear o guardar la imagen.
            const Errores e = comparar(salidaCPU, salidaGPU);
            peorError = std::max(peorError, e.maximo);
#else
            medirCPU();
#endif
            cpu.push_back(tiempoCPU);
            csv << "CPU," << nombreFiltro(o.filtro) << ',' << imagen.ancho << ',' << imagen.alto << ','
                << o.k << ",0,0," << r << ',' << tiempoCPU
                << ",,,," << tiempoCPU << ",,,,,ms\n";
#ifdef PRACTICA_WITH_CUDA
            h2d.push_back(t.h2d_ms); kernel.push_back(t.kernel_ms);
            d2h.push_back(t.d2h_ms); total.push_back(t.total_ms);
            transferencias.push_back(100.0 * (t.h2d_ms + t.d2h_ms) / t.total_ms);
            csv << "CUDA," << nombreFiltro(o.filtro) << ',' << imagen.ancho << ',' << imagen.alto << ','
                << o.k << ',' << o.hilos << ',' << gpu.bloques() << ',' << r
                << ",," << t.h2d_ms << ',' << t.kernel_ms << ',' << t.d2h_ms
                << ',' << t.total_ms << ',' << e.maximo << ',' << e.medio << ','
                << TOLERANCIA << ',' << (e.valido ? "true" : "false") << ",ms\n";
            csv.flush();
            if (!e.valido)
                throw std::runtime_error("CPU/GPU difieren. Se conservo el CSV del fallo.");
            std::cout << "Repeticion " << r << ": CPU=" << tiempoCPU
                      << " ms, kernel=" << t.kernel_ms << " ms, error max="
                      << e.maximo << '\n';
#else
            std::cout << "Repeticion " << r << ": CPU=" << tiempoCPU << " ms\n";
#endif
        }
        // Estadisticas fuera del cronometro; los datos originales permanecen en raw.csv.
        auto resumen = abrir(o.prefijo + "_summary.csv");
        resumen << "metrica,unidad,mediana,q1,q3,iqr\n";
        registrarResumen(resumen, "cpu", "ms", cpu);
#ifdef PRACTICA_WITH_CUDA
        registrarResumen(resumen, "h2d", "ms", h2d);
        registrarResumen(resumen, "kernel", "ms", kernel);
        registrarResumen(resumen, "d2h", "ms", d2h);
        registrarResumen(resumen, "total_gpu", "ms", total);
        registrarResumen(resumen, "proporcion_transferencias", "porcentaje", transferencias);
        // Cociente de medianas, no mediana de cocientes.
        const double medianaCPU = percentil(cpu, 0.5);
        const double medianaKernel = percentil(kernel, 0.5);
        const double medianaTotal = percentil(total, 0.5);
        metadatos << "Aceleracion computo (cociente medianas): "
                  << (medianaKernel > 0 ? medianaCPU / medianaKernel : NAN)
                  << "\nAceleracion con transferencias (cociente medianas): "
                  << (medianaTotal > 0 ? medianaCPU / medianaTotal : NAN)
                  << "\nPeor error absoluto: " << peorError << '\n';
        guardarVisual(o.prefijo + "_gpu.pgm", imagen, salidaGPU, o.filtro);
        std::cout << "VALIDACION CPU-GPU CORRECTA\n";
#else
        (void)peorError;
#endif
        guardarVisual(o.prefijo + "_cpu.pgm", imagen, salidaCPU, o.filtro);
        metadatos << "Estado: finalizado correctamente\n";
        csv.flush(); resumen.flush(); metadatos.flush();
        if (!csv || !resumen || !metadatos)
            throw std::runtime_error("Error de escritura en resultados.");
        std::cout << "Resultados guardados con prefijo: " << o.prefijo << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
}
