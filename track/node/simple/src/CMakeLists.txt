#set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../../arduino-cmake/cmake/ArduinoToolchain.cmake)

cmake_minimum_required(VERSION 2.8)

#print_board_list()
#print_programmer_list()

set(TARGET_NAME node_simple)
set(TARGET_BOARD leonardo)
set(${TARGET_NAME}_PORT /dev/tty.usbmodemfd141)

project( ${TARGET_NAME}_proj )

file(GLOB_RECURSE ${TARGET_NAME}_SRCS *.cpp)
file(GLOB_RECURSE ${TARGET_NAME}_HDRS *.hpp)



set(NANOPB_SRC_ROOT_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/../../nanopb")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${NANOPB_SRC_ROOT_FOLDER}/cmake)
find_package( Nanopb REQUIRED )

NANOPB_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../proto/simple_command.proto)

include_directories(${NANOPB_INCLUDE_DIRS})




set( ${TARGET_NAME}_SRCS  ${${TARGET_NAME}_SRCS} ${PROTO_SRCS})
set( ${TARGET_NAME}_HDRS  ${${TARGET_NAME}_HDRS} ${PROTO_HDRS})



generate_arduino_firmware(${TARGET_NAME} BOARD ${TARGET_BOARD} NO_AUTOLIBS)
