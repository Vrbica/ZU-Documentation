cmake_minimum_required(VERSION 3.16)
project(ZU_Documentation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(EDSDK_DIR "${CMAKE_SOURCE_DIR}/EDSDK")

add_executable(ZU_Documentation
    main.cpp
    mainwindow.cpp
    mainwindow.h
    canoncontroller.cpp
    canoncontroller.h
)

target_include_directories(ZU_Documentation PRIVATE
    ${EDSDK_DIR}/Header
)

target_link_libraries(ZU_Documentation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    "${EDSDK_DIR}/x64/EDSDK.lib"
)

# Copy EDSDK DLLs next to the executable
add_custom_command(TARGET ZU_Documentation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${EDSDK_DIR}/x64/EDSDK.dll"
        "${EDSDK_DIR}/x64/EdsImage.dll"
        $<TARGET_FILE_DIR:ZU_Documentation>
)
