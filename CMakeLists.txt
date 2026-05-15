cmake_minimum_required(VERSION 3.15)
project(Radiance3DCore)

include(FetchContent)

# ── pybind11 ──────────────────────────────────
FetchContent_Declare(pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v2.11.1
)
FetchContent_MakeAvailable(pybind11)

# ── GLFW ─────────────────────────────────────
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# ── Python ────────────────────────────────────
# Development.Module is the only component needed for a pybind11 extension.
# It works on all platforms including manylinux containers where
# Development.Embed / Python_LIBRARIES are intentionally absent.
find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
find_package(OpenGL REQUIRED)

# ── Bullet3 ───────────────────────────────────
set(BUILD_UNIT_TESTS     OFF CACHE BOOL "" FORCE)
set(BUILD_CPU_DEMOS      OFF CACHE BOOL "" FORCE)
set(BUILD_OPENGL3_DEMOS  OFF CACHE BOOL "" FORCE)
set(BUILD_BULLET2_DEMOS  OFF CACHE BOOL "" FORCE)
set(BUILD_EXTRAS         OFF CACHE BOOL "" FORCE)
FetchContent_Declare(bullet
    GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
    GIT_TAG        3.25
)
FetchContent_MakeAvailable(bullet)

# ── Build target ──────────────────────────────
pybind11_add_module(radiance3d engine.cpp glad/src/glad.c)

target_include_directories(radiance3d PRIVATE
    glad/include
    ${bullet_SOURCE_DIR}/src
)

target_link_libraries(radiance3d PRIVATE
    pybind11::module
    BulletDynamics
    BulletCollision
    LinearMath
    glfw
    OpenGL::GL
)

target_link_options(radiance3d PRIVATE
    -static
    -static-libgcc
    -static-libstdc++
)

# ── Post-build: copy to project root ──────────
add_custom_command(TARGET radiance3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:radiance3d>
        ${CMAKE_SOURCE_DIR}/
)

# ── Install (wheel packaging) ─────────────────
# Harmless locally unless cmake --install is explicitly run (it isn't).
# Required on CI for cibuildwheel to bundle the module into the .whl.
install(TARGETS radiance3d DESTINATION .)