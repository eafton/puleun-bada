cmake_minimum_required(VERSION 3.8.0)

include(GenerateExportHeader)

set(PBUIGTK2_SRC
    pbui-css-parser.c
    pbui-cairo-render.c
)

set(PBUIGTK2_HDR
    pbui-css-parser.h
    pbui-cairo-render.h
    pbui.h
)

add_library(pbui-gtk2 SHARED ${PBUIGTK2_SRC} ${PBUIGTK2_HDR})

generate_export_header(pbui-gtk2 EXPORT_FILE_NAME pbui-gtk2-export.h)

target_include_directories(pbui-gtk2 PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(pbui-gtk2 PUBLIC PkgConfig::GTK2_PC PkgConfig::GTK3_PC)

target_compile_options(pbui-gtk2 PRIVATE ${GTK2_PC_CFLAGS_OTHER} ${GTK3_PC_CFLAGS_OTHER})
target_compile_options(pbui-gtk2 PRIVATE -Wno-deprecated-declarations)
target_compile_definitions(pbui-gtk2 PUBLIC GLIB_DISABLE_DEPRECATION_WARNINGS)

set_target_properties(pbui-gtk2 PROPERTIES
    C_STANDARD 90
    C_STANDARD_REQUIRED TRUE
    C_EXTENSIONS OFF
    VERSION 1.0.0
    SOVERSION 1
)

install(TARGETS pbui-gtk2
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(FILES ${PBUIGTK2_HDR} ${CMAKE_CURRENT_BINARY_DIR}/pbui-gtk2-export.h
    DESTINATION include/libpbui-gtk2
)
