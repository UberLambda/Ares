# Ares.Core - The engine core

string(TOUPPER "${CMAKE_BUILD_TYPE}" BUILD_TYPE)

# ===== Source files and base build options/steps ==============================

if(BUILD_TYPE MATCHES "DEBUG|DEBWITHRELINFO")
    message("Debug/RelWithDebInfo build, WIN32 console window will be shown")
    set(ARES_WIN32)
else()
    # Make Release/RelWithDebInfo builds not have a console window
    set(ARES_WIN32 WIN32)
endif()

add_executable(Ares.Core ${ARES_WIN32}
    Main.cc Core.cc
    Task/TaskScheduler.cc
    Data/FileIO.cc Data/ResourceLoader.cc Data/ConfigResourceParser.cc
    Visual/Window.cc Visual/GLFW.cc Visual/InputMapper.cc
    Debug/Log.cc Debug/DebugModule.cc
    Mem/Mem.cc Mem/MallocOverrides.cc Mem/NewOverrides.cc
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


# ===== Libraries to link, build flags =========================================

add_subdirectory(3rdparty/)

target_link_libraries(Ares.Core PUBLIC
    glfw
    glm
    boost_context
    concurrentqueue
    vulkan
    flextgl
    tinyformat
    tinygltf
    plthook
)

if(TARGET rpmalloc)
    message("Using rpmalloc for memory allocation")
    set(ARES_HAS_RPMALLOC true)

    set(RPMALLOC_LIBS rpmalloc)
    if(WIN32)
        # Required for IAT hooking, see "Mem/rpmalloc/Hooks.cc"
        set(RPMALLOC_LIBS ${RPMALLOC_LIBS} psapi)
    endif()
    target_link_libraries(Ares.Core PUBLIC ${RPMALLOC_LIBS})
else()
    message("rpmalloc disabled, falling back to standard allocator")
    set(ARES_HAS_RPMALLOC false)
endif()

if(NOT MSVC)
    # Use POSIX threads everywhere but MSVC, incl. MinGW
    set(ARES_HAS_PTHREADS true)
endif()

if(BUILD_TYPE MATCHES RELEASE|RELWITHDEBINFO)
    # Force -DNDEBUG on release, relwithdebinfo builds
    target_compile_definitions(Ares.Core PUBLIC
        NDEBUG
    )
endif()

# ===== BuildConfig.h.in =======================================================

include(TestBigEndian)
test_big_endian(ARES_PLATFORM_BIG_ENDIAN)

configure_file(BuildConfig.h.in "${CMAKE_CURRENT_BINARY_DIR}/Ares/BuildConfig.h"
               ESCAPE_QUOTES
               @ONLY
               NEWLINE_STYLE UNIX)
target_include_directories(Ares.Core PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")


