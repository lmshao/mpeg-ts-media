cmake_minimum_required(VERSION 3.22)
project(ts_media)

set(CMAKE_CXX_STANDARD 17)

aux_source_directory(src SRCS)

add_executable(${PROJECT_NAME} ${SRCS})
