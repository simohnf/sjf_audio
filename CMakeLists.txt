# sjf_audio/CMakeLists.txt
cmake_minimum_required(VERSION 3.22)

# 1. Register the JUCE modules relative to this submodule
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/sjf_helpers")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/sjf_dsp")
juce_add_module("${CMAKE_CURRENT_SOURCE_DIR}/sjf_gui")

# 2. Add include paths (if needed outside standard JUCE module conventions)
target_include_directories(sjf_helpers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/sjf_helpers")
target_include_directories(sjf_dsp     INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/sjf_dsp")
target_include_directories(sjf_gui     INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/sjf_gui")