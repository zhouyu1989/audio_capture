cmake_minimum_required(VERSION 2.8)
project(audio_capture)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I..   -Iinclude -std=c++11 -O2 -g \
-DADB_HOST=0 -DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=${ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE} -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I..   -Iinclude  -O2 -g -DADB_HOST=0 \
-DROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE=${ROKIDOS_BOARDCONFIG_STORAGE_DEVICETYPE} -Wall -Wno-unused-parameter -D_XOPEN_SOURCE -D_GNU_SOURCE")


set(COMMON_FLAGS "-include rokid/rokid_types.h")

add_subdirectory(rokidrecordserver)
add_subdirectory(rokidrecordclient)
