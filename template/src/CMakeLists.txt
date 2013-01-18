cmake_minimum_required(VERSION 2.8)


set(TARGET_NAME helloworld)
project( ${TARGET_NAME}_proj )

find_package( OpenCV REQUIRED )

set(SOURCES
  main.cpp
)

set(HEADERS
)


add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS} )


