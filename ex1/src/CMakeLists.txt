cmake_minimum_required(VERSION 2.8)


set(TARGET_NAME ex1)
project( ${TARGET_NAME}_proj )

find_package( OpenCV 2 REQUIRED )

set(SOURCES
  main.cpp
  cartoon.cpp
)

set(HEADERS
  cartoon.hpp
)


add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS} )


