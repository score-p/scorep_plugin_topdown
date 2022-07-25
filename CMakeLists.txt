cmake_minimum_required(VERSION 3.15)
project(scorep_plugin_topdown)

set(NITRO_POSITION_INDEPENDENT_CODE ON CACHE INTERNAL "")
add_subdirectory(lib/scorep_plugin_cxx_wrapper)

add_library(topdown_plugin MODULE
    src/plugin.cpp
    include/plugin.hpp
    src/metric.cpp
    include/metric.hpp
)

target_include_directories(topdown_plugin PUBLIC include)
target_compile_features(topdown_plugin PUBLIC cxx_std_17)
target_link_libraries(topdown_plugin PUBLIC Scorep::scorep-plugin-cxx)
install(
    TARGETS topdown_plugin
    LIBRARY DESTINATION lib
)
