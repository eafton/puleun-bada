cmake_minimum_required(VERSION 3.8.0)
project(pb LANGUAGES C)

set(CMAKE_C_STANDARD 90)

add_definitions(-DGLIB_DISABLE_DEPRECATION_WARNINGS)
add_compile_options(-Wno-deprecated-declarations)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK2_PC REQUIRED IMPORTED_TARGET gtk+-2.0)
pkg_check_modules(GTK3_PC REQUIRED IMPORTED_TARGET gtk+-3.0)

add_subdirectory("pbsndvol")
add_subdirectory("pbisoburn")
add_subdirectory("pbrar")
add_subdirectory("pbvidcap")
add_subdirectory("pbcswizard")
