cmake_minimum_required(VERSION 3.20)
project(luacomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 23)
#set(CMAKE_C_CLANG_TIDY "clang-tidy;-checks=performance-*")

add_executable(${PROJECT_NAME} 
  src/main.cpp
  src/lib.hh
  src/tokenizer.hh
)
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
