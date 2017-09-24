cmake_minimum_required(VERSION 3.5)

# SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Build setting")
set(CMAKE_CXX_STANDARD 11)
project(NTRIPCLIENT)

add_executable(ntripclient 
    ntripclient.c
    serial.c    
)

target_link_libraries(
    ntripclient
)

install(TARGETS ntripclient DESTINATION bin)
