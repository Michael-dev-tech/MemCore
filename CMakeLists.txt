cmake_minimum_required(VERSION 3.16)
project(MemCore)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(MemCore main.cpp)

# Am adăugat librăriile de cripto OpenSSL aici:
target_link_libraries(MemCore PRIVATE Qt6::Widgets OpenSSL::SSL OpenSSL::Crypto)