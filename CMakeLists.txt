
cmake_minimum_required(VERSION 3.0)

if(NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 17)
endif()


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "-g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "-O2")
endif()


project(st80 C CXX)

find_package(SDL2 REQUIRED)

add_executable(imgswapper
        misc/imageswapper.c)

add_executable(st80
        src/bitblt.cpp
        src/interpreter.cpp
        src/objmemory.cpp
        src/vm.cpp
        src/main.cpp)

target_include_directories(st80 PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS})

target_link_libraries(st80 -lSDL2main -lSDL2)