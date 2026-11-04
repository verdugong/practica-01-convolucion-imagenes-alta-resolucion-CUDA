cmake_minimum_required(VERSION 3.24)

# Arquitectura de la NVIDIA GeForce RTX 3070.
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 86)
endif()

project(Practica1CUDA LANGUAGES CXX CUDA)

find_package(CUDAToolkit REQUIRED)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Utilizar Release para medir el rendimiento.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilacion" FORCE)
endif()

# Filtros secuenciales y funciones compartidas.
add_library(practica_cpu
    src/cpu/media_cpu.cpp
    src/cpu/nitidez_cpu.cpp
    src/cpu/laplaciano_cpu.cpp
    src/common/imagen.cpp
    src/common/validacion.cpp
    src/common/filtros.cpp
)

target_include_directories(practica_cpu PUBLIC include)

# Mantener operaciones numericas comparables entre CPU y GPU.
if(MSVC)
    target_compile_options(practica_cpu PRIVATE /W4 /fp:strict)
else()
    target_compile_options(practica_cpu PRIVATE
        -Wall -Wextra -Wpedantic -ffp-contract=off
    )
endif()

# Kernels y gestion de memoria y tiempos CUDA.
add_library(practica_gpu
    src/gpu/media_gpu.cu
    src/gpu/nitidez_gpu.cu
    src/gpu/laplaciano_gpu.cu
    src/gpu/filtros_gpu.cu
)

target_include_directories(practica_gpu PUBLIC include)
target_link_libraries(practica_gpu PUBLIC CUDA::cudart)
target_compile_options(practica_gpu PRIVATE
    $<$<COMPILE_LANGUAGE:CUDA>:--fmad=false>
)

# Programa principal.
add_executable(practica1 src/main.cpp)
target_link_libraries(practica1 PRIVATE practica_cpu practica_gpu)
target_compile_definitions(practica1 PRIVATE PRACTICA_WITH_CUDA=1)

# Pruebas de correccion.
enable_testing()

add_executable(prueba_cpu tests/prueba_cpu.cpp)
target_link_libraries(prueba_cpu PRIVATE practica_cpu)
add_test(NAME referencia_cpu COMMAND prueba_cpu)

add_executable(prueba_gpu tests/prueba_gpu.cpp)
target_link_libraries(prueba_gpu PRIVATE practica_cpu practica_gpu)
add_test(NAME equivalencia_cpu_gpu COMMAND prueba_gpu)
