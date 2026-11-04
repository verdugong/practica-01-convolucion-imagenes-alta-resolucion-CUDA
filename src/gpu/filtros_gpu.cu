// Administracion CUDA comun: reservas fuera del cronometro y medicion sincronizada.
#include "filtros_gpu.hpp"
#include "kernels.cuh"
#include <cuda_runtime.h>
#include <chrono>
#include <sstream>
#include <stdexcept>

namespace practica {
namespace {
using Reloj = std::chrono::steady_clock;
double milisegundos(Reloj::time_point inicio, Reloj::time_point fin)
{
    return std::chrono::duration<double, std::milli>(fin - inicio).count();
}
void comprobar(cudaError_t estado, const char* operacion)
{
    if (estado != cudaSuccess)
        throw std::runtime_error(std::string(operacion) + ": " +
                                 cudaGetErrorString(estado));
}
} // namespace

// Impl oculta los tipos CUDA a C++ y libera recursos incluso ante excepciones.
struct FiltroGPU::Impl {
    Filtro filtro = Filtro::Media;
    int ancho = 0, alto = 0, k = 0, hilos = 0, bloques = 0;
    std::size_t cantidad = 0, bytes = 0;
    float* entrada = nullptr;
    float* salida = nullptr;
    cudaEvent_t inicio = nullptr, fin = nullptr;
    cudaDeviceProp propiedades{};
    int runtime = 0, driver = 0;
    ~Impl()
    {
        // No lanzar excepciones desde un destructor, incluso tras un fallo CUDA.
        if (inicio) cudaEventDestroy(inicio);
        if (fin) cudaEventDestroy(fin);
        if (entrada) cudaFree(entrada);
        if (salida) cudaFree(salida);
    }
};
FiltroGPU::FiltroGPU(int ancho, int alto, int k, int hilos, Filtro filtro)
    : p_(std::make_unique<Impl>())
{
    if (ancho <= 0 || alto <= 0 || (k != 3 && k != 9 && k != 21) ||
        (hilos != 128 && hilos != 256 && hilos != 512))
        throw std::invalid_argument("Dimensiones, mascara o hilos invalidos.");
    if (filtro != Filtro::Media && k != 3)
        throw std::invalid_argument("Nitidez y laplaciano requieren mascara 3.");
    p_->filtro = filtro;
    comprobar(cudaSetDevice(0), "Seleccionar GPU");
    comprobar(cudaGetDeviceProperties(&p_->propiedades, 0), "Consultar GPU");
    comprobar(cudaRuntimeGetVersion(&p_->runtime), "Consultar CUDA runtime");
    comprobar(cudaDriverGetVersion(&p_->driver), "Consultar CUDA driver");
    if (hilos > p_->propiedades.maxThreadsPerBlock ||
        hilos > p_->propiedades.maxThreadsDim[0])
        throw std::runtime_error("La GPU no admite esta cantidad de hilos.");
    p_->ancho = ancho; p_->alto = alto; p_->k = k; p_->hilos = hilos;
    p_->cantidad = static_cast<std::size_t>(ancho) * alto;
    // Redondear hacia arriba garantiza un hilo para cada pixel.
    const auto bloques = (p_->cantidad + hilos - 1) / hilos;
    if (bloques > static_cast<std::size_t>(p_->propiedades.maxGridSize[0]))
        throw std::runtime_error("La imagen supera el limite de bloques en X.");
    p_->bloques = static_cast<int>(bloques);
    p_->bytes = p_->cantidad * sizeof(float);
    comprobar(cudaMalloc(reinterpret_cast<void**>(&p_->entrada), p_->bytes),
              "Reservar entrada GPU");
    comprobar(cudaMalloc(reinterpret_cast<void**>(&p_->salida), p_->bytes),
              "Reservar salida GPU");
    comprobar(cudaEventCreate(&p_->inicio), "Crear evento inicial");
    comprobar(cudaEventCreate(&p_->fin), "Crear evento final");
}
FiltroGPU::~FiltroGPU() = default;

TiemposGPU FiltroGPU::ejecutar(const std::vector<float>& entrada,
                            std::vector<float>& salida)
{
    if (entrada.size() != p_->cantidad || salida.size() != p_->cantidad ||
        &entrada == &salida)
        throw std::invalid_argument("Reservar arreglos separados del tamano correcto.");
    comprobar(cudaDeviceSynchronize(), "Sincronizar antes de medir");
    TiemposGPU tiempos;
    // Reloj de CPU para transferencias bloqueantes y tiempo conjunto.
    // Host usa memoria pageable (std::vector), sin solapamiento.
    const auto t0 = Reloj::now();
    comprobar(cudaMemcpy(p_->entrada, entrada.data(), p_->bytes,
                         cudaMemcpyHostToDevice), "Transferencia CPU-GPU");
    comprobar(cudaDeviceSynchronize(), "Finalizar transferencia CPU-GPU");
    const auto t1 = Reloj::now();

    comprobar(cudaEventRecord(p_->inicio), "Registrar inicio");
    switch (p_->filtro) {
        case Filtro::Media:
            lanzarMedia(p_->entrada,p_->salida,p_->ancho,p_->alto,p_->k,p_->bloques,p_->hilos); break;
        case Filtro::Nitidez:
            lanzarNitidez(p_->entrada,p_->salida,p_->ancho,p_->alto,p_->bloques,p_->hilos); break;
        case Filtro::Laplaciano:
            lanzarLaplaciano(p_->entrada,p_->salida,p_->ancho,p_->alto,p_->bloques,p_->hilos); break;
        default: throw std::invalid_argument("Filtro invalido.");
    }
    comprobar(cudaGetLastError(), "Lanzar kernel");
    comprobar(cudaEventRecord(p_->fin), "Registrar fin");
    comprobar(cudaEventSynchronize(p_->fin), "Finalizar kernel");
    const auto t2 = Reloj::now();
    comprobar(cudaMemcpy(salida.data(), p_->salida, p_->bytes,
                         cudaMemcpyDeviceToHost), "Transferencia GPU-CPU");
    const auto t3 = Reloj::now();

    float kernel_ms = 0.0f;
    comprobar(cudaEventElapsedTime(&kernel_ms, p_->inicio, p_->fin),
              "Leer tiempo del kernel");
    tiempos.h2d_ms = milisegundos(t0, t1);
    tiempos.kernel_ms = kernel_ms;
    tiempos.d2h_ms = milisegundos(t2, t3);
    tiempos.total_ms = milisegundos(t0, t3);
    return tiempos;
}
int FiltroGPU::bloques() const { return p_->bloques; }
std::string FiltroGPU::descripcion() const
{
    std::ostringstream s;
    s << "GPU: " << p_->propiedades.name
      << "\nVRAM total MiB: " << p_->propiedades.totalGlobalMem / (1024.0 * 1024.0)
      << "\nCompute capability: " << p_->propiedades.major << '.'
      << p_->propiedades.minor
      << "\nCUDA runtime: " << p_->runtime
      << "\nCUDA driver API: " << p_->driver
      << "\nCompilador NVCC: " << __CUDACC_VER_MAJOR__ << '.'
      << __CUDACC_VER_MINOR__ << '.' << __CUDACC_VER_BUILD__
      << "\nHilos por bloque: " << p_->hilos
      << "\nBloques en X: " << p_->bloques << '\n';
    return s.str();
}
} // namespace practica
