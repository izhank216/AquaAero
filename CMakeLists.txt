cmake_minimum_required(VERSION 3.15)
project(AquaAero LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ------------------- Find packages -------------------
if(WIN32 AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# SDL2
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)

# Vulkan
find_package(Vulkan REQUIRED)

# Unicorn
find_path(UNICORN_INCLUDE_DIR unicorn/unicorn.h)
find_library(UNICORN_LIBRARY unicorn)
if(NOT UNICORN_INCLUDE_DIR OR NOT UNICORN_LIBRARY)
    message(FATAL_ERROR "Unicorn not found! Install Unicorn dev package or set UNICORN_INCLUDE_DIR/UNICORN_LIBRARY.")
endif()

# ImGui
if(NOT DEFINED IMGUI_DIR)
    set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/thirdparty/imgui")
endif()
if(NOT EXISTS "${IMGUI_DIR}/imgui.h")
    message(FATAL_ERROR "ImGui not found in ${IMGUI_DIR}")
endif()

# ------------------- Source files -------------------
set(SRC
    main.cpp
    gui.cpp
    emulator.cpp
)

# ------------------- Add executable -------------------
add_executable(${PROJECT_NAME} ${SRC})

# ------------------- Include directories -------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
    ${UNICORN_INCLUDE_DIR}
    ${IMGUI_DIR}
)

# ------------------- Link libraries -------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${Vulkan_LIBRARIES}
    ${UNICORN_LIBRARY}
)

# ------------------- ImGui compilation -------------------
# ImGui implementation files
set(IMGUI_IMPL
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer.cpp
)

target_sources(${PROJECT_NAME} PRIVATE ${IMGUI_IMPL})

# ------------------- Definitions -------------------
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE "_CRT_SECURE_NO_WARNINGS")
endif()
