cmake_minimum_required(VERSION 3.20)
project(BOOTLOADER)

add_definitions(-DUNICODE)
add_executable(drive_reader drive_reader.cpp)
install(TARGETS drive_reader DESTINATION ${CMAKE_SOURCE_DIR})

add_executable(drive_writer drive_writer.cpp)
install(TARGETS drive_writer DESTINATION ${CMAKE_SOURCE_DIR})
