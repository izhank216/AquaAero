cmake_minimum_required(VERSION 3.16)
project(AquaAeroEmulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Sources
set(SOURCES
    main.cpp
    gui.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    # Add SDL2/SDL2_image includes if needed
    # Add Vulkan includes
)

# Find packages
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(Vulkan REQUIRED)

# Add executable
add_executable(AquaAero ${SOURCES})

# Link libraries
target_link_libraries(AquaAero
    PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
    SDL2_image::SDL2_image
    Vulkan::Vulkan
    # Add Unicorn library path if needed
)

# Platform-specific settings
if(WIN32)
    set_target_properties(AquaAero PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release
    )
elseif(APPLE)
    set_target_properties(AquaAero PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release
    )
else()
    set_target_properties(AquaAero PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
