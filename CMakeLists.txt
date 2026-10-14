cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)

# --- DISABLE AUDIO & NETWORK ---
set(SFML_BUILD_AUDIO OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK OFF CACHE BOOL "" FORCE)
# --------------------------------------------------
FetchContent_MakeAvailable(SFML)

find_package(OpenGL REQUIRED)

add_executable(main 
    src/main.cpp
    src/glyph.cpp
    src/opengl_loader.cpp
)

# Tell compiler where to find gl.h
target_include_directories(main PRIVATE 
    include 
)

target_compile_features(main PRIVATE cxx_std_17)


target_link_libraries(main 
    PRIVATE 
        SFML::Graphics
        OpenGL::GL # Link system OpenGL
)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)

# Give the program the absolute font path
target_compile_definitions(main PRIVATE FONT_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/fonts/cmuntt/")
