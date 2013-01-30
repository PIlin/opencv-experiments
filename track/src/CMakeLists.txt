cmake_minimum_required(VERSION 2.8)


set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../opencv/build_cpp11/")

set(TARGET_NAME track)
project( ${TARGET_NAME}_proj )

find_package( OpenCV 2 REQUIRED )
find_package(Boost 1.52.0 REQUIRED COMPONENTS system)

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")


add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS})

include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../cvblob/cvBlob"  "${CMAKE_CURRENT_BINARY_DIR}/cvblob")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../cvblob/cvBlob")
target_link_libraries( ${TARGET_NAME} cvblob ${Boost_LIBRARIES} ${OpenCV_LIBRARIES})

