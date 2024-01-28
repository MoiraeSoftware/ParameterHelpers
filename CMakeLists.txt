cmake_minimum_required(VERSION 3.20)

project(ParameterHelpers VERSION 0.0.1 LANGUAGES CXX
        DESCRIPTION "JUCE module for helping with parameters in binding and GUI"
        HOMEPAGE_URL "https://github.com/MoiraeSoftware/parameter_helpers")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)

if (NOT COMMAND juce_add_module)
    message(FATAL_ERROR "JUCE must be added to your project before parameter_helpers!")
endif ()

# this makes the assumption the current directory is named parameter_helpers
juce_add_module("${CMAKE_CURRENT_LIST_DIR}")
add_library(MoiraeSoftware::ParameterHelpers ALIAS parameter_helpers)