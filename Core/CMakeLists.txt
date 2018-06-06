# Ares.Core - The engine core

add_executable(Ares.Core WIN32
    Main.cc
    Task/TaskScheduler.cc
    Data/FileIO.cc
    Visual/Window.cc Visual/GLFW.cc Visual/InputMapper.cc
    Gfx/Vulkan.cc
    Debug/Log.cc
)
set_target_properties(Ares.Core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/"
    OUTPUT_NAME Ares
    CXX_STANDARD 14
)

option(ARES_STRIP_EXECUTABLE "Strip Ares executables in release build" ON)
if(ARES_STRIP_EXECUTABLE)
    string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)
    if(BUILD_TYPE MATCHES RELEASE AND NOT MSVC)
        message("Release binary, stripping executables")

        set(STRIP_COMMAND strip)
        if(NOT "${CMAKE_CXX_COMPILER_TARGET}" STREQUAL "")
            set(STRIP_COMMAND "${CMAKE_CXX_COMPILE_TARGET}-${STRIP_EXECUTABLE}")
        endif()
        add_custom_command(TARGET Ares.Core POST_BUILD
                           COMMAND ${STRIP_COMMAND} "$<TARGET_FILE:Ares.Core>")
    else()
        message("Debug/RelWithDebInfo binary, *NOT* stripping executables")
    endif()

else()
    message("Stripping disabled, *NOT* stripping executables")
endif()


add_subdirectory(3rdparty/)
target_link_libraries(Ares.Core PUBLIC
    glfw
    glm
    boost_context
    concurrentqueue
    vulkan
    tinyformat
)
