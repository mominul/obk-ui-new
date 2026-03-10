cmake_minimum_required(VERSION 3.16)
project(obk-topbar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Qt5
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

# Source files
set(SOURCES
    main.cpp
    TopBar.cpp
)

set(HEADERS
    TopBar.h
)

# Windows acrylic helper (API inspired by Win32-Acrylic-Effect)
if(WIN32)
    list(APPEND SOURCES AcrylicHelper.cpp)
    list(APPEND HEADERS AcrylicHelper.h)
endif()

# Platform-specific sources
if(APPLE)
    # Enable Objective-C++ for macOS
    enable_language(OBJCXX)
    list(APPEND SOURCES TopBar_mac.mm)
endif()

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link Qt5 libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        dwmapi
        user32
    )
endif()

if(APPLE)
    # Link macOS frameworks
    target_link_libraries(${PROJECT_NAME} PRIVATE
        "-framework AppKit"
        "-framework Cocoa"
        "-framework Foundation"
    )

    # Set Objective-C++ flags for .mm files
    set_source_files_properties(TopBar_mac.mm PROPERTIES
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

# Install target
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
