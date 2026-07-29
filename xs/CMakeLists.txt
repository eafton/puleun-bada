cmake_minimum_required(VERSION 3.10)

project(msstyles-gtk2-engine LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)

find_path(LIBPE_INCLUDE_DIR libpe/pe.h PATHS /usr/local/include /usr/include /opt/local/include $ENV{LIBPE_ROOT}/include $ENV{HOME}/.local/include PATH_SUFFIXES libpe "")

find_library(LIBPE_LIBRARY pe PATHS /usr/local/lib /usr/lib /usr/lib/x86_64-linux-gnu /usr/lib/i386-linux-gnu /opt/local/lib $ENV{LIBPE_ROOT}/lib $ENV{HOME}/.local/lib)

if (NOT LIBPE_INCLUDE_DIR OR NOT LIBPE_LIBRARY)
  message(FATAL_ERROR "libpe library was not found.")
endif()

set(LIBPE_INCLUDE_DIRS ${LIBPE_INCLUDE_DIR})
set(LIBPE_LIBRARIES ${LIBPE_LIBRARY})

pkg_get_variable(GTK2_BINARY_VERSION gtk+-2.0 gtk_binary_version)
pkg_get_variable(GTK2_LIBDIR gtk+-2.0 libdir)

add_library(msstyles MODULE msstyles_rc_style.c msstyles_style.c msstyles_theme_main.c msstyles_parser.c msstyles_draw.c)

target_include_directories(msstyles PRIVATE ${GTK2_INCLUDE_DIRS} ${LIBPE_INCLUDE_DIRS})
target_compile_options(msstyles PRIVATE ${GTK2_CFLAGS_OTHER})
target_compile_options(msstyles PRIVATE -Wno-deprecated-declarations)
target_compile_definitions(msstyles PUBLIC GLIB_DISABLE_DEPRECATION_WARNINGS)
set_target_properties(msstyles PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED TRUE C_EXTENSIONS OFF)
target_link_libraries(msstyles ${GTK2_LIBRARIES} ${LIBPE_LIBRARIES})

install(TARGETS msstyles LIBRARY DESTINATION ${GTK2_LIBDIR}/gtk-2.0/${GTK2_BINARY_VERSION}/engines)
