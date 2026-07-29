pkg_check_modules(CSWIZ_PC REQUIRED IMPORTED_TARGET libgphoto2 libinsane)

file(GLOB CZWIZ_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(pbcswizard ${CZWIZ_SRC})

target_link_libraries(pbcswizard PRIVATE PkgConfig::GTK3_PC PkgConfig::CSWIZ_PC)
