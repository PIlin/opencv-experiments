cmake_minimum_required(VERSION 2.8)
project (track_emul)

set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../opencv/build_cpp11/")



set(TARGET_NAME track_emul)
project( ${TARGET_NAME}_proj )

find_package( OpenCV 2 REQUIRED )

set(SOURCES
  main.cpp
)

set(HEADERS
)


set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")

#add_subdirectory(cvblob)

add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS})
#target_link_libraries( ${TARGET_NAME} cvblob)

#include_directories(cvblob/cvBlob)
#add_executable(red cvblob/samples/red_object_tracking.cpp)
#target_link_libraries( red ${OpenCV_LIBS} cvblob)
