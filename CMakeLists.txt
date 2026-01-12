cmake_minimum_required(VERSION 3.15)

project(AlexianPlancke)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1 ) 

option(SDL_SHARED "" OFF)
option(SDL_STATIC "" ON )
add_subdirectory(libs/SDL-release-3.2.24)

add_subdirectory(libs/geGL-master)

add_executable(${PROJECT_NAME} 
  src/main.cpp
  )

target_link_libraries(${PROJECT_NAME} PUBLIC
  SDL3::SDL3
  geGL::geGL
  )

add_custom_target(run ./${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC 
  src/
  libs/glm
  )
