pkg_check_modules(VIDCAP_PC REQUIRED IMPORTED_TARGET gthread-2.0 gstreamer-video-1.0 libcanberra-gtk3)

file(GLOB PBVIDCAP_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(pbvidcap ${PBVIDCAP_SRC})

target_link_libraries(pbvidcap PRIVATE PkgConfig::GTK3_PC PkgConfig::VIDCAP_PC)
