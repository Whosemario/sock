cmake_minimum_required(VERSION 2.6)

project(sock)

file(GLOB SRC RELATIVE ${PROJECT_SOURCE_DIR} *.c)

add_executable(sock ${SRC})

