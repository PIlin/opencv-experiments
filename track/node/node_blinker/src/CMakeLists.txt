#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../../arduino-cmake/cmake/ArduinoToolchain.cmake)

cmake_minimum_required(VERSION 2.8)

#print_board_list()
#print_programmer_list()

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../arduino_libs/XBee)

set(TARGET_NAME node_blinker)
set(TARGET_BOARD leonardo)

#set(${TARGET_NAME}_PORT /dev/tty.usbmodemfd131)
#set(${TARGET_NAME}_PORT /dev/tty.usbmodemfa141)
set(${TARGET_NAME}_PORT /dev/tty.usbmodemfa1331)

project( ${TARGET_NAME}_proj )

file(GLOB_RECURSE ${TARGET_NAME}_SRCS *.cpp *.c)
file(GLOB_RECURSE ${TARGET_NAME}_HDRS *.hpp *.h)



set(NANOPB_SRC_ROOT_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../nanopb")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_SRC_ROOT_FOLDER}/cmake)
find_package( Nanopb REQUIRED )

NANOPB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../proto/simple_command.proto)

include_directories(${NANOPB_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})



set( ${TARGET_NAME}_SRCS  ${${TARGET_NAME}_SRCS} ${PROTO_SRCS})
set( ${TARGET_NAME}_HDRS  ${${TARGET_NAME}_HDRS} ${PROTO_HDRS})



generate_arduino_firmware(${TARGET_NAME} BOARD ${TARGET_BOARD} NO_AUTOLIBS)
