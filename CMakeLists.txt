cmake_minimum_required(VERSION 3.20)
project(Kaleidoscope LANGUAGES C CXX)

find_package(LLVM 20.1 REQUIRED CONFIG)
message(STATUS "Using LLVMConfig.cmake in : ${LLVM_DIR}")
set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/bin)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/lib)

list(APPEND CMAKE_MODULE_PATH ${LLVM_CMAKE_DIR})
include(TableGen)
include(AddLLVM)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(
    -Weverything
    -Werror
    -Wno-c++98-compat-pedantic
    -Wno-c++98-compat
    -Wno-switch-default
    -Wno-padded
    -Wno-weak-vtables
  )
else ()
  add_compile_options(-Wall -Werror)
endif ()

if (DCMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-g)
endif ()

add_subdirectory(src)
