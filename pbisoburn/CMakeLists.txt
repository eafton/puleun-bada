pkg_check_modules(ISOBURN_PC REQUIRED IMPORTED_TARGET "libisoburn-1 libisofs-1 libburn-1")

file(GLOB PBISOBURN_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" )

add_executable(pbisoburn ${PBISOBURN_SRC})

target_link_libraries(pbisoburn PRIVATE PkgConfig::GTK3_PC PkgConfig::ISOBURN_PC)
