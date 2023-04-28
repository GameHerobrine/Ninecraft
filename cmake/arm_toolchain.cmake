set(CMAKE_SYSTEM_NAME Linux)

if (NOT ${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "arm")
    set(CMAKE_SYSTEM_PROCESSOR arm)
    set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mthumb")

set(CMAKE_FIND_ROOT_PATH /usr)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)