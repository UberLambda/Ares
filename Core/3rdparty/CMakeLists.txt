# == GLFW (3.x) == Upstream: https://github.com/glfw/glfw == License: zlib
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw/)

# == glm == Upstream: https://github.com/g-truc/glm == License: MIT
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm/")

# == Boost (context) == Upstream: https://www.boost.org/ == License: Boost 1.0
#find_package(Boost REQUIRED COMPONENTS context)
#add_library(boost INTERFACE)
#target_include_directories(boost INTERFACE ${BOOST_INCLUDE_DIRS})
#target_link_libraries(boost INTERFACE ${BOOST_LIBRARIES})

# == boost_context_ftl (forked boost::context from FiberTaskingLib)
# == Upstream: https://github.com/RichieSams/FiberTaskingLib
# == License: Apache2 (see file: boost_context_ftl/LICENSE.md)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/boost_context_ftl")
add_subdirectory(boost_context_ftl/)

# == moodycamel::concurrentqueue
# == Upstream: https://github.com/cameron314/concurrentqueue
# == License: Boost 1.0
add_library(concurrentqueue INTERFACE)
target_include_directories(concurrentqueue INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/concurrentqueue/")

# == Vulkan (+ vulkan-hpp)
# == Upstream: https://www.khronos.org/vulkan/ [imported via find script]
# == License: Apache2 (see system headers)
find_package(Vulkan REQUIRED)
add_library(vulkan INTERFACE)
target_include_directories(vulkan INTERFACE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan INTERFACE ${Vulkan_LIBRARIES})

# == OpenGL (libGL) 3.3+
# == Upstream: https://www.khronos.org/opengl/ [imported via find script]
# == License: N/A
find_package(OpenGL 3.3 REQUIRED)
add_library(opengl INTERFACE)
target_include_directories(opengl INTERFACE ${OPENGL_INCLUDE_DIR})
target_link_libraries(opengl INTERFACE ${OPENGL_gl_LIBRARY})

# == tinyformat
# == Upstream: https://github.com/c42f/tinyformat [in-tree]
# == License: Boost 1.0
add_library(tinyformat INTERFACE)
target_include_directories(tinyformat INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/tinyformat/")
