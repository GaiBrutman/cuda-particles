cmake_minimum_required(VERSION 3.18)

option(CPU_ONLY           "Build without CUDA — CPU simulation, no NVCC required" OFF)
option(USE_OPENGL_BACKEND "Build the OpenGL rendering backend"                    OFF)

if(CPU_ONLY)
    project(cuda_particles LANGUAGES CXX)
else()
    project(cuda_particles LANGUAGES CXX CUDA)
    set(CMAKE_CUDA_STANDARD 17)
endif()

set(CMAKE_CXX_STANDARD 17)

# ── App sources (always compiled) ─────────────────────────────────────────────
set(APP_SOURCES
    src/main.cpp
    src/app/app.cpp
    src/app/config.cpp
)

add_executable(particles ${APP_SOURCES})
target_include_directories(particles PRIVATE src third_party/stb)

# ── CPU-only path ─────────────────────────────────────────────────────────────
if(CPU_ONLY)
    target_sources(particles PRIVATE
        src/simulation/cpu/particle_system_cpu.cpp
        src/simulation/cpu/integrate_cpu.cpp
        src/simulation/cpu/forces_cpu.cpp
        src/simulation/cpu/collision_cpu.cpp
        src/render/cpu/cpu_renderer.cpp
        src/render/offscreen/frame_exporter.cpp
        third_party/stb/stb_impl.cpp
    )
    target_compile_definitions(particles PRIVATE CPU_ONLY)

# ── CUDA path ─────────────────────────────────────────────────────────────────
else()
    target_sources(particles PRIVATE
        src/simulation/particle_system.cu
        src/simulation/kernels/integrate.cu
        src/simulation/kernels/forces.cu
        src/simulation/kernels/collision.cu
        src/render/offscreen/offscreen_renderer.cu
        src/render/offscreen/frame_exporter.cpp
    )
    set_target_properties(particles PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

    # ── OpenGL backend (optional, CUDA path only) ─────────────────────────────
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
endif()
