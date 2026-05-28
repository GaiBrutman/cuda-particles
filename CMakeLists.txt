cmake_minimum_required(VERSION 3.18)
project(cuda_particles LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

option(USE_OPENGL_BACKEND "Build the OpenGL rendering backend" OFF)

# ── Common sources ────────────────────────────────────────────────────────────
set(SIM_SOURCES
    src/simulation/particle_system.cu
    src/simulation/kernels/integrate.cu
    src/simulation/kernels/forces.cu
    src/simulation/kernels/collision.cu
)

set(OFFSCREEN_SOURCES
    src/render/offscreen/offscreen_renderer.cu
    src/render/offscreen/frame_exporter.cpp
)

set(APP_SOURCES
    src/main.cpp
    src/app/app.cpp
    src/app/config.cpp
)

# ── Executable ────────────────────────────────────────────────────────────────
add_executable(particles
    ${APP_SOURCES}
    ${SIM_SOURCES}
    ${OFFSCREEN_SOURCES}
)

target_include_directories(particles PRIVATE src)

set_target_properties(particles PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
)

# ── OpenGL backend (optional) ─────────────────────────────────────────────────
if(USE_OPENGL_BACKEND)
    find_package(OpenGL REQUIRED)
    find_package(glfw3  REQUIRED)

    target_sources(particles PRIVATE
        src/render/opengl/gl_renderer.cpp
        src/render/opengl/gl_interop.cu
        src/render/opengl/shader.cpp
        src/ui/imgui_overlay.cpp
    )

    target_link_libraries(particles PRIVATE OpenGL::GL glfw)
    target_compile_definitions(particles PRIVATE USE_OPENGL)
endif()
