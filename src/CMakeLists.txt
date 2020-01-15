cmake_minimum_required(VERSION 2.8)

# VitaSDK defines
if( NOT DEFINED CMAKE_TOOLCHAIN_FILE )
  if( DEFINED ENV{VITASDK} )
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

# Project start
set(VITA_APPNAME vectoroids)
set(VITA_TITLEID "VECT00001")
set(VITA_VERSION "01.00")

project(${VITA_APPNAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

# Flags and includes
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions")
set(DATA_PREFIX \"app0:data/\")

add_definitions(-DVITA)
add_definitions(-D__SOUND)
add_definitions(-DJOY_YES)
add_definitions(-DDATA_PREFIX=${DATA_PREFIX})

include_directories(
)

link_directories(
  ${CMAKE_CURRENT_BINARY_DIR}
)

# Builds
add_executable(${VITA_APPNAME}
source/vectoroids.c
)


target_link_libraries(${VITA_APPNAME}
  SDL_image
  SDL_mixer
  SDL
  mikmod
  vorbisfile
  vorbis
  ogg
  jpeg
  png
  vita2d
  z
  m
  SceAppUtil_stub
  SceAudio_stub
  SceCommonDialog_stub
  SceCtrl_stub
  SceDisplay_stub
  SceGxm_stub
  SceSysmodule_stub
  SceTouch_stub
  SceHID_stub
)

# Create Vita artifacts
vita_create_self(eboot.bin ${VITA_APPNAME} SAFE)
vita_create_vpk(${VITA_APPNAME}.vpk ${VITA_TITLEID} eboot.bin
  VERSION ${VITA_VERSION}
  NAME ${VITA_APPNAME}
  FILE sce_sys sce_sys
  FILE data data

)