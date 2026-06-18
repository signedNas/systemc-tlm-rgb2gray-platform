cmake_minimum_required(VERSION 3.15)
project(rgb2gray CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED SYSTEMC_HOME)
    set(SYSTEMC_HOME "$ENV{SYSTEMC_HOME}")
endif()
if(NOT SYSTEMC_HOME)
    set(SYSTEMC_HOME "/usr/local/systemc")
endif()

message(STATUS "SYSTEMC_HOME = ${SYSTEMC_HOME}")

find_library(SYSTEMC_LIB
    NAMES systemc
    PATHS
        "${SYSTEMC_HOME}/lib"
        "${SYSTEMC_HOME}/lib-linux64"
        "${SYSTEMC_HOME}/lib-linux"
    REQUIRED
)
message(STATUS "SystemC lib  = ${SYSTEMC_LIB}")

add_executable(rgb2gray src/main.cpp)

target_include_directories(rgb2gray PRIVATE
    "${SYSTEMC_HOME}/include"
    "${CMAKE_SOURCE_DIR}/src"
)

target_link_libraries(rgb2gray "${SYSTEMC_LIB}")

set_target_properties(rgb2gray PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
