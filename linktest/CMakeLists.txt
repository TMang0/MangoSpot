cmake_minimum_required(VERSION 3.13)
project(cspot_switch_linktest CXX)

set(BELL_DISABLE_MQTT ON CACHE BOOL "" FORCE)
set(BELL_DISABLE_WEBSERVER ON CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../external/cspot/cspot cspot_build)

add_executable(linktest main.cpp)
target_link_libraries(linktest PRIVATE cspot)
