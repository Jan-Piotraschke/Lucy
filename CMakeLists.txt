cmake_minimum_required(VERSION 3.28)

# ------------------------------------------------------------------------------
# 1) Set vcpkg toolchain and target triplet (must come before project())
# ------------------------------------------------------------------------------
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "Vcpkg toolchain" FORCE)
set(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "vcpkg target triplet" FORCE)

project(
  Lucy
  VERSION 1.0
  DESCRIPTION "Lucy Dev App"
  LANGUAGES CXX
)

# ------------------------------------------------------------------------------
# 2) Build type, C++20, and shared libs
# ------------------------------------------------------------------------------
# By default, build in Release mode to minimize size
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build SFML, TGUI, etc. as shared libraries
set(BUILD_SHARED_LIBS ON)

# Enable Interprocedural Optimization (LTO) if the compiler supports it
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

# ------------------------------------------------------------------------------
# 3) Set the output directory for the binary
# ------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ------------------------------------------------------------------------------
# 4) Fetch SFML and TGUI if not installed via vcpkg
# ------------------------------------------------------------------------------
include(FetchContent)

# -- Use SFML 3.0.0
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG        3.0.0
    GIT_SHALLOW    ON
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SFML)

# Force TGUI to use the SFML_GRAPHICS backend
set(TGUI_BACKEND SFML_GRAPHICS CACHE STRING "" FORCE)
set(TGUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TGUI_BUILD_TESTS OFF CACHE BOOL "" FORCE)

# TGUI 1.8.0
FetchContent_Declare(
    TGUI
    GIT_REPOSITORY https://github.com/texus/TGUI.git
    GIT_TAG        v1.8.0
    GIT_SHALLOW    ON
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(TGUI)

# ------------------------------------------------------------------------------
# 5) Ensure OpenCV and quirc are found
# ------------------------------------------------------------------------------
set(OpenCV_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/arm64-osx/share/opencv4")
set(quirc_DIR   "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/installed/arm64-osx/share/quirc")

find_package(quirc CONFIG REQUIRED)
find_package(OpenCV CONFIG REQUIRED)

# ------------------------------------------------------------------------------
# 6) Add Kiss FFT (lightweight, self-contained FFT library)
# ------------------------------------------------------------------------------
add_library(kiss_fft STATIC
    src/modules/kiss_fft/kiss_fft.cpp
)

target_include_directories(kiss_fft PRIVATE src/modules/kiss_fft)

# ------------------------------------------------------------------------------
# 7) Create the executable and link libraries
# ------------------------------------------------------------------------------
add_executable(main
    src/main.cpp
    src/modules/tile/hexagon_tile.cpp
    src/modules/mesh/mesh.cpp
    src/modules/kamon_fourier/kamon_fourier.cpp
    src/modules/logs_report/logs_report.cpp
)

target_compile_features(main PRIVATE cxx_std_20)

target_link_libraries(main
    PRIVATE
        SFML::Graphics
        TGUI::TGUI
        ${OpenCV_LIBS}
        quirc::quirc
        kiss_fft
)

set_target_properties(main PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
