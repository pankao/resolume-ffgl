add_library(ffgl-plugin-timewave STATIC)
add_library(ffgl::plugin::timewave ALIAS ffgl-plugin-timewave)
target_sources(ffgl-plugin-timewave PRIVATE TimeWave.h TimeWave.cpp)
target_link_libraries(ffgl-plugin-timewave PRIVATE ffgl::sdk)

install(
    TARGETS     ffgl-plugin-timewave
    EXPORT      ffgl--plugin-timewave-targets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
