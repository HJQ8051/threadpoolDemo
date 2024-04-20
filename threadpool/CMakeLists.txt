cmake_minimum_required(VERSION 3.15)
project(Threadpool)

include_directories(${HEADPATH})
aux_source_directory(. SRC_LIST)
set(LIBRARY_OUTPUT_PATH ${LIBPATH})
add_library(threadpool STATIC ${SRC_LIST})

find_package(Threads REQUIRED)
target_link_libraries(threadpool PRIVATE ${CMAKE_THREAD_LIBS_INIT})