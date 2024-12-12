cmake_minimum_required(VERSION 3.18)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_PROGRESS TRUE
  GIT_TAG 0.9.9.8)

FetchContent_GetProperties(glm)

if(NOT glm_POPULATED)
  set(GLM_FORCE_CXX11 1)
  FetchContent_MakeAvailable(glm)
  set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
endif()

set(glm_INCLUDE ${glm_SOURCE_DIR})

message(STATUS "GLM Should Be Downloaded")
