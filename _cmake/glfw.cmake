cmake_minimum_required(VERSION 3.18)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
)

FetchContent_GetProperties(glfw)

if(NOT glfw_POPULATED)
  FetchContent_MakeAvailable(glfw)
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
endif()

set(glfw_INCLUDE ${glfw_SOURCE_DIR}/include)

message(STATUS "GLFW Should Be Downloaded")
