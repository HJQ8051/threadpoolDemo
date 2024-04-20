cmake_minimum_required(VERSION 3.15)
project(Test)

include_directories(${HEADPATH})
aux_source_directory(. SRC_LIST)
set(EXECUTABLE_OUTPUT_PATH ${EXEPATH})
link_directories(${LIBPATH})
link_libraries(threadpool)
add_executable(test ${SRC_LIST})