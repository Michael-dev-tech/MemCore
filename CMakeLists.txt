cmake_minimum_required(VERSION 3.16)
project(MemCore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Instruct CMake to automatically handle Qt's special background processes
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find the Qt6 Widgets module on your system
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Compile our main file
add_executable(MemCore main.cpp)

# Link the Qt6 library to our MemCore application
target_link_libraries(MemCore PRIVATE Qt6::Widgets)