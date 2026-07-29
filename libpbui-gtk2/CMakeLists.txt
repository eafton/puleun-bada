file(GLOB PBUIGTK2_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(libpbui-gtk2 ${PBUIGTK2_SRC})

target_link_libraries(libpbui-gtk2 PRIVATE PkgConfig::GTK2_PC)
