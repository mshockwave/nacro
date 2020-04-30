cmake_minimum_required(VERSION 3.4.4)
project(clang-new-macro)

set(CMAKE_CXX_STANDARD 14)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIR})

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)

find_package(Clang REQUIRED CONFIG)
include_directories(${CLANG_INCLUDE_DIRS})
list(APPEND CMAKE_MODULE_PATH "${CLANG_CMAKE_DIR}")

# Handle RTTI stuff, which often leads to error
if(NOT ${LLVM_ENABLE_RTTI})
  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR
      "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
      "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
    message(STATUS "Disable RTTI")
  elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
    message(STATUS "Disable RTTI")
  endif()
  # Do not give any flags for other less widely used
  # compilers
endif()

option(NACRO_ENABLE_UNITTESTS "Enable unit tests for naco" OFF)
# FIXME: What about static lib?
option(NACRO_BUILD_LIB
       "Enable building a shared library containing all nacro functions" OFF)

set(_SOURCE_FILES
    NacroPragmaHandler.cpp
    NacroRule.cpp
    NacroParsers.cpp
    NacroExpander.cpp
    NacroVerifier.cpp
    )

add_llvm_library(NacroPlugin MODULE
                 ${_SOURCE_FILES}
                 PLUGIN_TOOL clang)

#add_subdirectory(Playground)

if(${NACRO_BUILD_LIB} OR ${NACRO_ENABLE_UNITTESTS})
  add_library(Nacro SHARED
              ${_SOURCE_FILES})
  target_link_libraries(Nacro
                        clangBasic
                        clangLex
                        clangAST)
endif()

if(${NACRO_ENABLE_UNITTESTS})
  add_subdirectory(unittest)
endif()
