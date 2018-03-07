cmake_minimum_required(VERSION 3.5)
project(wandstem_flash_utility)

set(CMAKE_CXX_STANDARD 11)

#include(serial-port/6_stream/CMakeLists.txt)

## Target
set(TEST_SRCS main.cpp serial-port/6_stream/serialstream.cpp Program.cpp Device.cpp XmodemPacket.cpp )
set(TEST_HDRS serial-port/6_stream/serialstream.h Program.h Device.h  XmodemPacket.h Exceptions.h)
add_executable(wandstem-flash ${TEST_SRCS} ${TEST_HDRS})

## Link libraries
set(BOOST_USE_STATIC_LIBS   ON)
set(BOOST_ROOT /usr/local)
set(BOOST_LIBS thread date_time system program_options)
find_package(Boost COMPONENTS ${BOOST_LIBS} REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(wandstem-flash ${Boost_LIBRARIES})
find_package(Threads REQUIRED)
target_link_libraries(wandstem-flash ${CMAKE_THREAD_LIBS_INIT})

#add_custom_target(wandstem_flash_utility COMMAND make -C ${wandstem_flash_utility_SOURCE_DIR}
#        CLION_EXE_DIR=${PROJECT_BINARY_DIR})