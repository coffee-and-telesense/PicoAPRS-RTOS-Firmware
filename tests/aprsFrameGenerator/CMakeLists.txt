add_executable(aprsTest sampleMain.c)

target_sources(aprsTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/sampleMain.c
)

target_link_libraries(aprsTest PRIVATE 
    stm32cubemx
    aprsFrameGenerator
)