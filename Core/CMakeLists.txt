# Ares.Core - The engine core

add_executable(Ares.Core WIN32
    Main.cc
)
set_target_properties(Ares.Core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/"
    OUTPUT_NAME Ares
    CXX_STANDARD 14
)
