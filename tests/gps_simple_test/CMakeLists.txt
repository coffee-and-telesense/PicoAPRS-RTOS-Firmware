add_executable(gps_simple_test test_gps.c)

target_sources(gps_simple_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/test_gps.c
)

set(GPS_NON_BLOCKING ON CACHE BOOL "Enable GPS Blocking/Non-Blocking Feature" FORCE)

target_link_libraries(gps_simple_test PRIVATE 
    stm32cubemx
    gps
)