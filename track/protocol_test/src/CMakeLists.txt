cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME protocol_test)
project( ${TARGET_NAME}_proj )

find_package(Boost 1.52.0 REQUIRED COMPONENTS system)

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../proto/simple_command.proto)



add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${PROTO_SRCS} ${PROTO_HDRS})
target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${PROTOBUF_LIBRARIES})

include_directories(${Boost_INCLUDE_DIRS})

