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
    Data/FileIO.cc Data/ResourceLoader.cc
    Resource/Gltf.cc Resource/Json.cc Resource/ShaderSrc.cc
    Visual/Window.cc Visual/GLFW.cc
    Input/InputMapper.cc Input/InputModule.cc
    Debug/Log.cc Debug/DebugModule.cc
    Mem/Mem.cc Mem/MallocOverrides.cc Mem/NewOverrides.cc
    Scene/Scene.cc
    Gfx/GfxModule.cc Gfx/GfxRenderer.cc Gfx/GfxPipeline.cc
    Gfx/GL33/Shader.cc Gfx/GL33/GBuffer.cc Gfx/GL33/MeshBuf.cc Gfx/GL33/Texture.cc Gfx/GL33/Backend.cc
    App/AppModule.cc
)
set_target_properties(Ares.Core PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/"
    OUTPUT_NAME Ares
    CXX_STANDARD 14

    ENABLE_EXPORTS ON # So that App.so/App.dll can link to Ares' executable
    DEFINE_SYMBOL "ARES_EXPORTS"
)
target_include_directories(Ares.Core PUBLIC ${PROJECT_SOURCE_DIR}) # So that you can #include <Core/...>

option(ARES_STRIP_EXECUTABLE "Strip Ares executables in release build" ON)
if(ARES_STRIP_EXECUTABLE)
    if(BUILD_TYPE MATCHES RELEASE AND NOT MSVC)
        message("Release binary, stripping executables")

        set(STRIP_COMMAND strip)
        if(NOT "${CMAKE_CXX_COMPILER_TARGET}" STREQUAL "")
            set(STRIP_COMMAND "${CMAKE_CXX_COMPILER_TARGET}-${STRIP_COMMAND}")
        endif()
        add_custom_command(TARGET Ares.Core POST_BUILD
                           COMMAND ${STRIP_COMMAND} "$<TARGET_FILE:Ares.Core>")
    else()
        message("Debug/RelWithDebInfo binary, *NOT* stripping executables")
    endif()

else()
    message("Stripping disabled, *NOT* stripping executables")
endif()


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

# #define ARES_SRCDIR_PATH_LENGTH; See: https://stackoverflow.com/a/40947954 and Debug/Log/Log.hh
# **WARNING** Make sure that CMake has been invoked with the full path to the
#             source code; if relative paths are used,
#             length(${PROJECT_SOURCE_DIR}/srcfile.cc) != length(__FILE__),
#             **potentially making `ARES_FILE_` in Debug/Log.hh point to garbage
#             data over the end of the __FILE__ string!!**
string(LENGTH "${PROJECT_SOURCE_DIR}/" SRCDIR_PATH_LENGTH)
target_compile_options(Ares.Core PRIVATE -DARES_SRCDIR_PATH_LENGTH=${SRCDIR_PATH_LENGTH})

# ===== BuildConfig.h.in =======================================================

include(TestBigEndian)
test_big_endian(ARES_PLATFORM_BIG_ENDIAN)

configure_file(BuildConfig.h.in "${CMAKE_CURRENT_BINARY_DIR}/Ares/BuildConfig.h"
               ESCAPE_QUOTES
               @ONLY
               NEWLINE_STYLE UNIX)
target_include_directories(Ares.Core PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")


