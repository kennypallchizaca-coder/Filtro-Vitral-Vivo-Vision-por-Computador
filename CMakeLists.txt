cmake_minimum_required(VERSION 3.16)
project(interciclo_vision LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED COMPONENTS
    core imgproc highgui imgcodecs videoio viz
)

add_executable(interciclo_vision Principal.cpp Mediciones.cpp)

target_include_directories(interciclo_vision PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(interciclo_vision PRIVATE ${OpenCV_LIBS})

if(WIN32)
    # psapi consulta la RAM del proceso; pdh proporciona los contadores GPU de Windows.
    target_link_libraries(interciclo_vision PRIVATE psapi pdh)
endif()

option(BUILD_TESTING "Compilar pruebas de la vista 3D" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_executable(prueba_vista3d tests/prueba_vista3d.cpp)
    target_include_directories(prueba_vista3d PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(prueba_vista3d PRIVATE ${OpenCV_LIBS})
    add_test(NAME textura_video_dinamica COMMAND prueba_vista3d)
endif()
