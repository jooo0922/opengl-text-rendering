cmake_minimum_required(VERSION 3.18)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
  GIT_PROGRESS TRUE
  GIT_TAG VER-2-10-2
)

FetchContent_GetProperties(freetype)

if(NOT freetype_POPULATED)
  FetchContent_MakeAvailable(freetype)
endif()

set(freetype_INCLUDE ${freetype_SOURCE_DIR}/include)

message(STATUS "FreeType Should Be Downloaded")
