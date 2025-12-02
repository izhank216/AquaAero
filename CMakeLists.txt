cmake_minimum_required(VERSION 3.16)
project(AquaAeroEmulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(SOURCES
    main.cpp
    gui.cpp
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    /usr/local/include  # Homebrew default for SDL2, SDL2_image
)

# Link directories
link_directories(
    /usr/local/lib
)

# Find SDL2
find_package(SDL2 REQUIRED)
if(SDL2_FOUND)
    message(STATUS "SDL2 include: ${SDL2_INCLUDE_DIRS}")
    message(STATUS "SDL2 lib: ${SDL2_LIBRARIES}")
endif()

# SDL2_image (manually)
set(SDL2_IMAGE_INCLUDE_DIR /usr/local/include)
set(SDL2_IMAGE_LIB /usr/local/lib/libSDL2_image.dylib)

# Vulkan
find_package(Vulkan REQUIRED)

# Add executable
add_executable(AquaAero ${SOURCES})

# Link libraries
target_link_libraries(AquaAero
    PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIB}
    Vulkan::Vulkan
)

# Include directories for compiler
target_include_directories(AquaAero
    PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIR}
)

# Output directory
set_target_properties(AquaAero PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release
)
