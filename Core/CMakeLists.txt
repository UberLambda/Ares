# Ares.Core - The engine core


# ===== Source files and base build options/steps ==============================

add_executable(Ares.Core WIN32
    Main.cc Core.cc
    Task/TaskScheduler.cc
    Data/FileIO.cc Data/ResourceLoader.cc Data/ConfigResourceParser.cc
    Visual/Window.cc Visual/GLFW.cc Visual/InputMapper.cc
    Debug/Log.cc
    Scene/Scene.cc
    Gfx/GfxModule.cc Gfx/GL33/Shader.cc
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


# ===== BuildConfig.h.in =======================================================

include(TestBigEndian)
test_big_endian(ARES_PLATFORM_BIG_ENDIAN)

configure_file(BuildConfig.h.in "${CMAKE_CURRENT_BINARY_DIR}/Ares/BuildConfig.h"
               ESCAPE_QUOTES
               @ONLY
               NEWLINE_STYLE UNIX)
target_include_directories(Ares.Core PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")


# ===== Comp generation ========================================================

find_package(PythonInterp 3.5 REQUIRED)
add_custom_target(Ares.Core.gencomps
    COMMAND ${PYTHON_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/Comp/gencomps.py"
                                 --indir "${CMAKE_CURRENT_SOURCE_DIR}/"
                                 --outdir "${CMAKE_CURRENT_BINARY_DIR}/Ares/"
    COMMENT "Running gencomps"
)
add_dependencies(Ares.Core Ares.Core.gencomps)

# Note: ${CMAKE_CURRENT_BINARY_DIR} already included by generating BuildConfig.h.in


# ===== Libraries to link ======================================================

add_subdirectory(3rdparty/)
target_link_libraries(Ares.Core PUBLIC
    glfw
    glm
    boost_context
    concurrentqueue
    vulkan
    flextgl
    tinyformat
)
