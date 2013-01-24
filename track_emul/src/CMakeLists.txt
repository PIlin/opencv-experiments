cmake_minimum_required(VERSION 2.8)
project (track_emul)

set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../opencv/build_cpp11/")

set(TARGET_NAME track_emul)
project( ${TARGET_NAME}_proj )

find_package( OpenCV 2 REQUIRED )

file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")

add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../cvblob/cvBlob"  "${CMAKE_CURRENT_BINARY_DIR}/cvblob")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../cvblob/cvBlob")
target_link_libraries( ${TARGET_NAME} cvblob)

