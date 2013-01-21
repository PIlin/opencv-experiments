cmake_minimum_required(VERSION 2.8)

set(OpenCV_DIR "../../../OpenCV-2.4.3/build_cpp11/")


set(TARGET_NAME ex2)
project( ${TARGET_NAME}_proj )

find_package( OpenCV REQUIRED )

set(SOURCES
	CameraCalibration.cpp
	GeometryTypes.cpp
	main.cpp
	Marker.cpp
	MarkerDetector.cpp
	TinyLA.cpp
)

set(HEADERS
	BGRAVideoFrame.h
	CameraCalibration.hpp
	GeometryTypes.hpp
	make_unique.hpp
	Marker.hpp
	MarkerDetectionFacade.hpp
	MarkerDetector.hpp
	TinyLA.hpp
)


set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++")

add_executable( ${TARGET_NAME} ${SOURCES} ${HEADERS})
target_link_libraries( ${TARGET_NAME} ${OpenCV_LIBS} )


