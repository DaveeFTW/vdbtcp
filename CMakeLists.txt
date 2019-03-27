cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(vdbtcp)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}  -std=c99 -fno-strict-aliasing")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wl,-q -Wall -O3 -nostdlib -fno-rtti -fno-exceptions")

add_executable(vdbtcp
  main.cpp 
  connection.cpp
  network.cpp
  server.cpp
  thread.cpp
  eventflag.cpp
  abi.cpp
  log.h
)

target_link_libraries(vdbtcp
  gcc
  vdb_user_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  SceNetCtl_stub_weak
  SceNet_stub_weak
)

add_custom_target(exports SOURCES exports.yml)

vita_create_self(vdbtcp.suprx vdbtcp CONFIG exports.yml UNSAFE)
