cmake_minimum_required(VERSION 3.21)
project(neural_rendering_ui_harness LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
enable_testing()
add_executable(neural-rendering-ui-harness
    ui-harness.cpp
    ../../rpcs3/rpcs3qt/neural_rendering_tab.cpp
    ../../rpcs3/rpcs3qt/neural_rendering_config.cpp
)
target_compile_features(neural-rendering-ui-harness PRIVATE cxx_std_20)
target_include_directories(neural-rendering-ui-harness PRIVATE ../../rpcs3/rpcs3qt)
target_link_libraries(neural-rendering-ui-harness PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
if(MSVC)
    target_compile_options(neural-rendering-ui-harness PRIVATE /utf-8)
endif()
set_target_properties(neural-rendering-ui-harness PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sandbox"
)
add_test(NAME neural-rendering-ui COMMAND neural-rendering-ui-harness)
set_tests_properties(neural-rendering-ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
