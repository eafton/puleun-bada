cmake_minimum_required(VERSION 3.8.0)

project(gtk2-engine-pbui LANGUAGES C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK2 REQUIRED gtk+-2.0)

pkg_get_variable(GTK2_BINARY_VERSION gtk+-2.0 gtk_binary_version)
pkg_get_variable(GTK2_LIBDIR gtk+-2.0 libdir)

set(GTK2_ENGINE_SRC
    pbui_rc_style.c
    pbui_style.c
    pbui_theme_main.c
)

add_library(pbui MODULE ${GTK2_ENGINE_SRC})

target_include_directories(pbui PRIVATE
    ${GTK2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/libpbui-gtk2
)

target_link_libraries(pbui
    ${GTK2_LIBRARIES}
    pbui-gtk2
)

target_compile_options(pbui PRIVATE ${GTK2_CFLAGS_OTHER})
target_compile_options(pbui PRIVATE -Wno-deprecated-declarations)
target_compile_definitions(pbui PUBLIC GLIB_DISABLE_DEPRECATION_WARNINGS)

set_target_properties(pbui PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS OFF
    PREFIX ""
    OUTPUT_NAME "libpbui"
)

install(TARGETS pbui LIBRARY DESTINATION ${GTK2_LIBDIR}/gtk-2.0/${GTK2_BINARY_VERSION}/engines)
