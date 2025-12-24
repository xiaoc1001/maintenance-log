cmake_minimum_required(VERSION 3.16)

project(MaintenanceLogQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

qt_standard_project_setup()

add_executable(MaintenanceLog
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ApiClient.h
    src/ApiClient.cpp
    src/DateUtils.h
    src/DateUtils.cpp
)

target_link_libraries(MaintenanceLog PRIVATE Qt6::Widgets Qt6::Network)
