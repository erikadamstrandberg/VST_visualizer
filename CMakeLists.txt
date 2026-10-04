cmake_minimum_required(VERSION 3.22)
project(VST_Visualizer_Suite VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)

message(STATUS "Fetching JUCE 8.0.6...")
FetchContent_MakeAvailable(JUCE)

# Enable folder grouping in IDEs (CLion, Visual Studio, Xcode)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

# Add VST plugins directory
add_subdirectory(plugins)
