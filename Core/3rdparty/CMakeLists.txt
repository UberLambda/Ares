# == GLFW (3.x) == Upstream: https://github.com/glfw/glfw == License: zlib
add_subdirectory(glfw/)

# == glm == Upstream: https://github.com/g-truc/glm == License: MIT
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/glm/")
