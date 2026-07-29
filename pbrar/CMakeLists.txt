pkg_check_modules(PBRAR_DEPS_PC REQUIRED IMPORTED_TARGET gio-2.0 libgcab-1.0 libarchive)

file(GLOB PBRAR_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" )

add_executable(pbrar ${PBRAR_SRC})

target_link_libraries(pbrar PRIVATE PkgConfig::GTK3_PC PkgConfig::PBRAR_DEPS_PC)
