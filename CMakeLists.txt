cmake_minimum_required(VERSION 3.0.0)

project(Open_Bookmark_Sync_Merger VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5Core)
find_package(Qt5Sql)

add_executable(${PROJECT_NAME} "main.cpp" "obsm.cpp" "obsm.h")

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Sql)
