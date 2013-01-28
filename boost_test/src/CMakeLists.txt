cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME boost_test)
project( ${TARGET_NAME}_proj )

FIND_PACKAGE(Boost REQUIRED)


file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")

add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS})


