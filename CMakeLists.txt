cmake_minimum_required(VERSION 3.16)

project(RTDemo)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR}/include/struct)
INCLUDE_DIRECTORIES(${SDL2_INCLUDE_DIRS})

find_package(OpenMP)
find_package(SDL2 REQUIRED)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

set(SOURCES main.cpp)

add_executable (RTDemo ${SOURCES})

target_link_libraries(RTDemo ${SDL2_LIBRARIES})
