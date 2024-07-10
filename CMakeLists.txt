cmake_minimum_required(VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project("MathExpressionParser")

add_library(${PROJECT_NAME} MathExpressionParser/MathExpressions.cpp)

add_subdirectory(Parser)
target_link_libraries(${PROJECT_NAME} PUBLIC Parser)
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}/Parser")