"""
Build and run the particle simulation, then display or export the result.

Works locally and in Google Colab.

Usage:
    python run_simulation.py                            # build + run + show animation
    python run_simulation.py --output anim.gif          # build + run + save GIF
    python run_simulation.py --particles 500000 --frames 120 --output /tmp/anim.mp4
    python run_simulation.py --no-build                 # skip build, re-use existing binary
"""

import argparse
import os
import subprocess
import sys


# ── helpers ──────────────────────────────────────────────────────────────────

def in_colab() -> bool:
    try:
        import google.colab  # noqa: F401
        return True
    except ImportError:
        return False


def build(project_root: str, cpu_only: bool) -> str:
    build_dir = os.path.join(project_root, "build_cpu" if cpu_only else "build")
    cmake_args = ["cmake", project_root, "-B", build_dir, "-DCMAKE_BUILD_TYPE=Release"]
    if cpu_only:
        cmake_args.append("-DCPU_ONLY=ON")

    print("── Configuring …")
    subprocess.run(cmake_args, check=True)

    print("── Building …")
    subprocess.run(["cmake", "--build", build_dir, "--parallel"], check=True)

    binary = os.path.join(build_dir, "particles")
    if not os.path.exists(binary):
        sys.exit(f"Build succeeded but binary not found at {binary}")
    return binary


def run_sim(binary: str, particles: int, frames: int, width: int, height: int,
            dt: float, output_dir: str) -> None:
    os.makedirs(output_dir, exist_ok=True)
    cmd = [
        binary,
        "--particles", str(particles),
        "--frames",    str(frames),
        "--width",     str(width),
        "--height",    str(height),
        "--dt",        str(dt),
        "--output",    output_dir,
    ]
    print(f"── Running simulation ({particles:,} particles, {frames} frames) …")
    subprocess.run(cmd, check=True)


def display_colab(frames_dir: str, every_n: int = 5) -> None:
    import glob
    from IPython.display import Image, display as ipy_display
    paths = sorted(glob.glob(os.path.join(frames_dir, "frame_*.png")))
    print(f"Displaying {len(paths) // every_n} of {len(paths)} frames (every {every_n}th)")
    for p in paths[::every_n]:
        ipy_display(Image(p))


# ── main ─────────────────────────────────────────────────────────────────────

def main():
    project_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

    parser = argparse.ArgumentParser(description="Build, run, and visualize the particle simulation")
    parser.add_argument("--no-build",   action="store_true", help="Skip cmake build step")
    parser.add_argument("--cpu-only",   action="store_true", default=True,
                        help="Build without CUDA (default: True)")
    parser.add_argument("--particles",  type=int,   default=100)
    parser.add_argument("--frames",     type=int,   default=300)
    parser.add_argument("--width",      type=int,   default=1280)
    parser.add_argument("--height",     type=int,   default=720)
    parser.add_argument("--dt",         type=float, default=0.016)
    parser.add_argument("--frames-dir", default=os.path.join(project_root, "frames"),
                        help="Where frames are written and read from")
    parser.add_argument("--output",     default=None,
                        help="Animation output file (.gif or .mp4). None = interactive.")
    args = parser.parse_args()

    if not args.no_build:
        binary = build(project_root, cpu_only=args.cpu_only)
    else:
        build_dir = os.path.join(project_root, "build_cpu" if args.cpu_only else "build")
        binary = os.path.join(build_dir, "particles")
        if not os.path.exists(binary):
            sys.exit(f"Binary not found at {binary}. Remove --no-build to compile first.")

    run_sim(binary, args.particles, args.frames, args.width, args.height,
            args.dt, args.frames_dir)

    visualize_script = os.path.join(project_root, "python", "visualize.py")
    viz_cmd = [sys.executable, visualize_script,
               "--frames", args.frames_dir,
               "--fps",    "30"]
    if args.output:
        viz_cmd += ["--output", args.output]

    if in_colab() and not args.output:
        display_colab(args.frames_dir)
    else:
        subprocess.run(viz_cmd, check=True)


if __name__ == "__main__":
    main()
