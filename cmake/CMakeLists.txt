cmake_minimum_required(VERSION 3.16)

project(sctp_sandbox)

include(../tools/CMakeCommon.cmake)

verifyCppCompiler(8 6)

find_library(SCTP_LIBRARY sctp)
find_package(Threads REQUIRED)

set(CMAKE_EXE_LINKER_FLAGS "-Wl,--export-dynamic,--no-as-needed")
target_link_libraries(${PROJECT_NAME} ${CMAKE_EXE_LINKER_FLAGS} Threads::Threads sctp rt SegFault)

