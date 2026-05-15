cmake_minimum_required(VERSION 3.15)
project(Radiance3DCore)

# ─────────────────────────────────────────────
# Detect environment: GitHub Actions sets CI=true automatically.
# Use this to switch between a wheel-ready release build and a
# fast local dev build without any manual flags.
# ─────────────────────────────────────────────
if(DEFINED ENV{CI} AND "$ENV{CI}" STREQUAL "true")
    message(STATUS "CI environment detected → using release/wheel build config")
    set(IS_CI_BUILD TRUE)
else()
    message(STATUS "Local environment detected → using dev build config")
    set(IS_CI_BUILD FALSE)
endif()

# PIC is required for manylinux wheel builds on CI; unnecessary overhead locally
if(IS_CI_BUILD)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

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
# Development.Module is the minimal component needed for wheel builds (CI).
# Plain Development is easier to locate in a full local Python install.
if(IS_CI_BUILD)
    find_package(Python COMPONENTS Interpreter Development.Module REQUIRED)
else()
    find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

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

# ── Post-build: copy .pyd/.so to project root ─
add_custom_command(TARGET radiance3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:radiance3d>
        ${CMAKE_SOURCE_DIR}/
)

# ── Install (wheel packaging) — CI only ───────
# The install() rule is what tells the wheel builder (scikit-build / cibuildwheel)
# where to place the module inside the .whl archive. Not needed locally.
if(IS_CI_BUILD)
    install(TARGETS radiance3d DESTINATION .)
endif()