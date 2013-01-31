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

generate_arduino_firmware(${TARGET_NAME} BOARD ${TARGET_BOARD})