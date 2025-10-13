cmake_minimum_required(VERSION 3.21)
project(MidiSequenceGenerator VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG master # Use latest JUCE for macOS 15 (Sequoia) fixes
)
FetchContent_MakeAvailable(juce)

# Enable VST3 only, no AU to keep it simple
option(BUILD_AU "Build AudioUnit" OFF)

add_subdirectory(Source)
