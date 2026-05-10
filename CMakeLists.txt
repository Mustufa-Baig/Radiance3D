cmake_minimum_required(VERSION 3.15)
project(Radiance3DCore)

# Force all static libraries to be compiled with Position Independent Code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_package(OpenGL REQUIRED)


# 1. Disable all of Bullets bloat so we only compile the core math and physics
set(BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

# 2. Download the official Bullet3 repository
FetchContent_Declare(
    bullet
    GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
    GIT_TAG 3.25 # Locking to a stable release
)
FetchContent_MakeAvailable(bullet)

# 4. Add GLAD source file directly to the engine compilation
pybind11_add_module(radiance3d engine.cpp glad/src/glad.c)

# 5. Tell the engine where the GLAD headers are
target_include_directories(radiance3d PRIVATE glad/include)

# Tell the compiler where to find Bullets header files
target_include_directories(radiance3d PRIVATE ${bullet_SOURCE_DIR}/src)

# 3. Add the specific Bullet targets to your engine link step
# (Make sure to replace your old target_link_libraries with this one!)
target_link_libraries(radiance3d PRIVATE 
    pybind11::module 
    BulletDynamics 
    BulletCollision 
    LinearMath
)

# 6. Link libraries and force static
target_link_libraries(radiance3d PRIVATE glfw OpenGL::GL)
target_link_options(radiance3d PRIVATE -static -static-libgcc -static-libstdc++)

# Automatically copy the compiled .pyd file to the root directory
add_custom_command(TARGET radiance3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    $<TARGET_FILE:radiance3d>
    ${CMAKE_SOURCE_DIR}/
)

# Tell the packager to bundle the compiled module into the final wheel
install(TARGETS radiance3d DESTINATION .)