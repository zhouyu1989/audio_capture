cmake_minimum_required(VERSION 2.8)
project(robotcap)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")


set(COMMON_FLAGS "-include rokid/rokid_types.h")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
aux_source_directory(. DIR_SRCS)

add_library(robotalsa SHARED  mixer.c pcm.c pipe.c pipe_util.c)
target_link_libraries(robotalsa android_utils android_cutils android_hardware)

add_executable(robotcap robotcap.c main.c)
target_link_libraries(robotcap  android_utils  android_cutils robotalsa pthread)


add_executable(mictoolservertest server.c)
target_link_libraries(mictoolservertest  android_utils  android_cutils robotalsa)
