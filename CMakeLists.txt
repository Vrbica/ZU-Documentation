cmake_minimum_required(VERSION 3.16)
project(ZU_Documentation VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(EDSDK_DIR "${CMAKE_SOURCE_DIR}/EDSDK")

# WIN32 hides the console window in release builds
add_executable(ZU_Documentation WIN32
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

# ---- Post-build: copy Canon SDK DLLs ----
add_custom_command(TARGET ZU_Documentation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${EDSDK_DIR}/x64/EDSDK.dll"
        "${EDSDK_DIR}/x64/EdsImage.dll"
        $<TARGET_FILE_DIR:ZU_Documentation>
    COMMENT "Copying Canon SDK DLLs..."
)

# ---- Post-build: deploy Qt DLLs (windeployqt) ----
find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
if(WINDEPLOYQT)
    add_custom_command(TARGET ZU_Documentation POST_BUILD
        COMMAND "${WINDEPLOYQT}"
            --no-translations
            --no-system-d3d-compiler
            --no-opengl-sw
            $<TARGET_FILE:ZU_Documentation>
        COMMENT "Running windeployqt..."
    )
endif()
