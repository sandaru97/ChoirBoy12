cmake_minimum_required(VERSION 3.10)
project(ChoirEffect)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

add_executable(choir_effect main.cpp)

target_include_directories(choir_effect PRIVATE ${SNDFILE_INCLUDE_DIRS} ${PORTAUDIO_INCLUDE_DIRS})
target_link_libraries(choir_effect ${SNDFILE_LIBRARIES} ${PORTAUDIO_LIBRARIES})