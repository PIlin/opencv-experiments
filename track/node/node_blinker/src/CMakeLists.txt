#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../../arduino-cmake/cmake/ArduinoToolchain.cmake)

cmake_minimum_required(VERSION 2.8)

set(TARGET_NAME node_blinker)

project( ${TARGET_NAME}_proj )

set(MRF_LIB_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../mrflib/)

file(GLOB_RECURSE SRCS *.cpp *.c)
file(GLOB_RECURSE HDRS *.hpp *.h)


list(APPEND SRCS ${MRF_LIB_PATH}/mrf24j.cpp)
list(APPEND HDRS ${MRF_LIB_PATH}/mrf24j.h)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../mrflib/)


set(NANOPB_SRC_ROOT_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../nanopb")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_SRC_ROOT_FOLDER}/cmake)
find_package( Nanopb REQUIRED )

NANOPB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../proto/simple_command.proto)
list(APPEND SRCS ${PROTO_SRCS})
list(APPEND HDRS ${PROTO_HDRS})
include_directories(${NANOPB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})


function (BUILD_FOR_BOARD TNAME)
	parse_generator_arguments(${TNAME} INPUT
						  ""                  # Options
						  "TID;BOARD;PORT"        # One Value Keywords
						  "SRCS;HDRS"  # Multi Value Keywords
						  ${ARGN})

	set(TBNAME ${TNAME}_${INPUT_TID}_${INPUT_BOARD})

	message(STATUS "Have target ${TBNAME}")

	generate_arduino_firmware(${TBNAME}
		BOARD ${INPUT_BOARD}
		SRCS ${INPUT_SRCS}
		HDRS ${INPUT_HDRS}
		PORT ${INPUT_PORT}
	)

	set_target_properties(${TBNAME} PROPERTIES
		COMPILE_DEFINITIONS "ARDUINO_BOARD_${INPUT_BOARD};ARDUINO_NODE_ID=${INPUT_TID}"
	)

endfunction()

# BUILD_FOR_BOARD(${TARGET_NAME}
# 	TID 1
# 	SRCS ${SRCS}
# 	HDRS ${HDRS}
# 	BOARD "atmega328"
# 	PORT "/dev/tty.usbserial-A9007M68"
# )

BUILD_FOR_BOARD(${TARGET_NAME}
	TID 2
	SRCS ${SRCS}
	HDRS ${HDRS}
	BOARD "leonardo"
	PORT "/dev/tty.usbmodemfa121"
)
