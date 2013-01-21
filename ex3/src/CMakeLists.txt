cmake_minimum_required(VERSION 2.8)


set(OpenCV_DIR "../../../opencv/build_cpp11/")


set(TARGET_NAME ex3)
project( ${TARGET_NAME}_proj )

find_package( OpenCV 2 REQUIRED )

set(SOURCES
  main.cpp
)

set(HEADERS
)


set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")

add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS} )


