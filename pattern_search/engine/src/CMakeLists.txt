cmake_minimum_required(VERSION 2.8)

set(OpenCV_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../../../opencv/build_cpp11/")

set(TARGET_NAME pattern_search_engine)
project(${TARGET_NAME}_proj )

find_package( OpenCV 2 REQUIRED )
find_package(Boost 1.52.0 REQUIRED COMPONENTS system)
find_package(OpenGL REQUIRED )

#file(GLOB_RECURSE SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)

set(SOURCES
	main.cpp
	CameraCalibration.cpp
	ARPipeline.cpp
	ARDrawingContext.cpp
)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${Boost_INCLUDE_DIRS})

add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${TARGET_NAME} ${OpenCV_LIBS})
target_link_libraries(${TARGET_NAME} ${Boost_LIBRARIES})
target_link_libraries(${TARGET_NAME} ${OPENGL_LIBRARIES} )

add_custom_target(update_cmake)


