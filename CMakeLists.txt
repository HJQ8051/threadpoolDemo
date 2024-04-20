cmake_minimum_required(VERSION 3.15)
project(Demo)

set(LIBPATH ${PROJECT_SOURCE_DIR}/build/lib)
set(EXEPATH ${PROJECT_SOURCE_DIR}/build/bin)
set(HEADPATH ${PROJECT_SOURCE_DIR}/include)

add_subdirectory(threadpool)
add_subdirectory(src)