function(sjf_add_juce_unit_test_target PLUGIN_NAME)
    cmake_parse_arguments(ARG          ""       ""       "MODULES;FLAGS" ${ARGN})

    set(TEST_TARGET_NAME "${PLUGIN_NAME}Tests")

    # 1. Create console application for testing
    juce_add_console_app(${TEST_TARGET_NAME}
            PRODUCT_NAME "${TEST_TARGET_NAME}"
    )

    target_include_directories(${TEST_TARGET_NAME}
            PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sjf_audio/UnitTester"
            "${CMAKE_CURRENT_SOURCE_DIR}/sjf_audio"
    )

    target_link_libraries(${TEST_TARGET_NAME}
            PRIVATE
            juce::juce_recommended_config_flags
            ${ARG_MODULES}

            PRIVATE
            ${ARG_FLAGS}
    )

    # 2. Add compile definitions
    target_compile_definitions(${TEST_TARGET_NAME}
            PRIVATE
            JucePlugin_Name="${TEST_TARGET_NAME}"
    )

    # 3. Generate JuceHeader for the test target
    juce_generate_juce_header(${TEST_TARGET_NAME})

    # 4. Automatically find test files using CMAKE_CURRENT_SOURCE_DIR
    file(GLOB_RECURSE SJF_TEST_FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/sjf_audio/*_tests.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/sjf_audio/*Tests.cpp"
    )

    # 5. Add target source files
    target_sources(${TEST_TARGET_NAME}
            PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sjf_audio/sjf_UnitTester/main.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/PluginProcessor.cpp"
            ${SJF_TEST_FILES}
    )


    # 7. Register with CTest
    enable_testing()
    add_test(NAME "Run_${TEST_TARGET_NAME}" COMMAND ${TEST_TARGET_NAME})
endfunction()