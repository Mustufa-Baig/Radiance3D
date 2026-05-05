cmake_minimum_required(VERSION 3.15)
project(Radiance3DCore)

include(FetchContent)

# 1. Fetch pybind11
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# 2. Fetch GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

# 3. Locate Python and OpenGL
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(OpenGL REQUIRED)

# 4. Add GLAD source file directly to the engine compilation
pybind11_add_module(radiance3d engine.cpp glad/src/glad.c)

# 5. Tell the engine where the GLAD headers are
target_include_directories(radiance3d PRIVATE glad/include)

# 6. Link libraries and force static
target_link_libraries(radiance3d PRIVATE glfw OpenGL::GL)
target_link_options(radiance3d PRIVATE -static -static-libgcc -static-libstdc++)