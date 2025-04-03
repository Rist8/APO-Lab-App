cmake_minimum_required(VERSION 3.16)

project(APO-Lab-App VERSION 0.3 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Core Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Core Gui)
if(WIN32)
    set(OpenCV_DIR
        "C:/opencv/build/x64/vc16/lib"
    )
endif()
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS} include ${Qt5Widgets_INCLUDE_DIRS})
link_directories(${Qt5Widgets_LIBRARY_DIRS})



set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        include/mainwindow.h
        src/imageviewer.cpp
        include/imageviewer.h
        include/histogramwidget.h
        src/histogramwidget.cpp
        include/rangestretchingdialog.h
        src/rangestretchingdialog.cpp
        include/customfilterdialog.h
        src/customfilterdialog.cpp
        include/directionselectiondialog.h
        src/directionselectiondialog.cpp
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(APO-Lab-App
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        include/bitwiseoperationdialog.h
        src/bitwiseoperationdialog.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET APO-Lab-App APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(APO-Lab-App SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(APO-Lab-App
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(APO-Lab-App PRIVATE Qt${QT_VERSION_MAJOR}::Widgets ${OpenCV_LIBS} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.APO-Lab-App)
endif()
set_target_properties(APO-Lab-App PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS APO-Lab-App
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(APO-Lab-App)
endif()
