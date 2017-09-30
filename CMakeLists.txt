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

install(
    TARGETS ntripclient 
    DESTINATION bin
    )
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/ntripclient.sh" 
    DESTINATION bin
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/ntripclient.conf"  
    DESTINATION /etc/default/
    )
install(
    FILES "${CMAKE_CURRENT_SOURCE_DIR}/ntripclient-init.sh"  
    DESTINATION /etc/init.d/
    PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ
    )
