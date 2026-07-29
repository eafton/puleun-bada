pkg_check_modules(PULSE_PC REQUIRED IMPORTED_TARGET libpulse libpulse-mainloop-glib)

file(GLOB PBSNDVOL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" )

add_executable(pbsndvol ${PBSNDVOL_SRC})

target_link_libraries(pbsndvol PRIVATE PkgConfig::GTK3_PC PkgConfig::PULSE_PC)
