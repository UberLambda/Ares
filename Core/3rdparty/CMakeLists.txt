# == GLFW (3.x) == Upstream: https://github.com/glfw/glfw == License: zlib
add_subdirectory(glfw/)

# == glm == Upstream: https://github.com/g-truc/glm == License: MIT
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm/")

# == Boost (context) == Upstream: https://www.boost.org/ == License: Boost 1.0
find_package(Boost REQUIRED COMPONENTS context)
add_library(boost INTERFACE)
target_include_directories(boost INTERFACE ${BOOST_INCLUDE_DIRS})
target_link_libraries(boost INTERFACE ${BOOST_LIBRARIES})
