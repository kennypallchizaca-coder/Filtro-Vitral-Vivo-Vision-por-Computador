cmake_minimum_required(VERSION 3.16)
project(interciclo_vision LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS
    core imgproc highgui videoio viz
)

add_executable(interciclo_vision Principal.cpp Mediciones.cpp)

target_include_directories(interciclo_vision PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(interciclo_vision PRIVATE ${OpenCV_LIBS})

if(WIN32)
    # PSAPI/PDH miden recursos; Comctl32 permite cerrar con seguridad el visor VTK integrado.
    target_link_libraries(interciclo_vision PRIVATE psapi pdh comctl32)
endif()

option(BUILD_TESTING "Compilar pruebas de Vitral Vivo, mediciones y vista 3D" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_executable(prueba_vista3d tests/prueba_vista3d.cpp)
    target_include_directories(prueba_vista3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(prueba_vista3d PRIVATE ${OpenCV_LIBS})
    if(WIN32)
        target_link_libraries(prueba_vista3d PRIVATE comctl32)
    endif()
    add_test(NAME textura_video_dinamica COMMAND prueba_vista3d)
    add_executable(prueba_sin_csv tests/prueba_sin_csv.cpp Mediciones.cpp)
    target_include_directories(prueba_sin_csv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(prueba_sin_csv PRIVATE ${OpenCV_LIBS})
    if(WIN32)
        target_link_libraries(prueba_sin_csv PRIVATE psapi pdh comctl32)
    endif()
    add_test(NAME ayuda_sin_csv COMMAND prueba_sin_csv ayuda)
    add_test(NAME benchmark_sin_archivos COMMAND prueba_sin_csv benchmark)
    add_executable(prueba_efecto tests/prueba_efecto.cpp)
    target_include_directories(prueba_efecto PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(prueba_efecto PRIVATE ${OpenCV_LIBS})
    add_test(NAME filtro_vitral COMMAND prueba_efecto)
endif()
