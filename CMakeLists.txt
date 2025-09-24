cmake_minimum_required(VERSION 3.5)

project(Robot VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt Widgets + Qt3D
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets 3DCore 3DRender 3DInput 3DExtras 3DLogic)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets 3DCore 3DRender 3DInput 3DExtras 3DLogic)


set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        initaldate.h
        presets.h
        presets.cpp
        core.h
        core.cpp
        visual.h
        visual.cpp
        app.h
        app.cpp
        doublespindelegate.h
        render3d.h
        render3d.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(Robot
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET Robot APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(Robot SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(Robot
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(Robot PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::3DCore
    Qt${QT_VERSION_MAJOR}::3DRender
    Qt${QT_VERSION_MAJOR}::3DInput
    Qt${QT_VERSION_MAJOR}::3DExtras
    Qt${QT_VERSION_MAJOR}::3DLogic
)

set_target_properties(Robot PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(Robot)
endif()
