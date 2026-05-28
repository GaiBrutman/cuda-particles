"""
Build and run the particle simulation, then display or export the result.

Works locally and in Google Colab.

Usage:
    python run_simulation.py                            # build + run + show animation
    python run_simulation.py --output anim.gif          # build + run + save GIF
    python run_simulation.py --particles 500000 --frames 120 --output /tmp/anim.mp4
    python run_simulation.py --no-build                 # skip build, re-use existing binary
    python run_simulation.py --profiling                # enable per-frame timing + plot
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


def build(project_root: str, cpu_only: bool, profiling: bool) -> str:
    build_dir = os.path.join(project_root, "build_cpu" if cpu_only else "build")
    cmake_args = [
        "cmake", project_root, "-B", build_dir,
        "-DCMAKE_BUILD_TYPE=Release",
        f"-DPROFILING={'ON' if profiling else 'OFF'}",
    ]
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
            dt: float, output_dir: str, profiling: bool) -> str | None:
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
    if profiling:
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(result.stdout)
        if result.stderr:
            print(result.stderr, file=sys.stderr)
        return result.stdout
    else:
        subprocess.run(cmd, check=True)
        return None


def plot_profiling(stdout: str) -> None:
    import re
    import numpy as np
    import matplotlib.pyplot as plt
    import matplotlib.ticker as ticker

    frame_re = re.compile(
        r'^(\d+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)',
        re.MULTILINE
    )
    rows = frame_re.findall(stdout)
    if not rows:
        print("No per-frame timing data found in output.")
        return

    frames_idx = [int(r[0])   for r in rows]
    sim        = [float(r[1]) for r in rows]
    render     = [float(r[2]) for r in rows]
    readback   = [float(r[3]) for r in rows]
    save       = [float(r[4]) for r in rows]

    phases = ['sim', 'render', 'readback', 'save']
    avgs   = [np.mean(sim), np.mean(render), np.mean(readback), np.mean(save)]
    colors = ['#4c8cbf', '#e0883a', '#59a869', '#d9534f']
    total_avg = sum(avgs)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(13, 4))
    fig.suptitle('Frame timing breakdown', fontsize=12, fontweight='bold')

    # ── Left: avg ms per phase ────────────────────────────────────────────────
    bars = ax1.bar(phases, avgs, color=colors, width=0.5, edgecolor='white')
    ax1.set_ylabel('avg ms / frame')
    ax1.set_title('Phase averages')
    for bar, v in zip(bars, avgs):
        ax1.text(bar.get_x() + bar.get_width() / 2, v + total_avg * 0.01,
                 f'{v:.3f}', ha='center', va='bottom', fontsize=9)
    ax1.set_ylim(0, total_avg * 1.15)
    ax1.yaxis.set_major_formatter(ticker.FormatStrFormatter('%.1f'))

    # ── Right: stacked frame time over run ────────────────────────────────────
    ax2.stackplot(frames_idx, sim, render, readback, save,
                  labels=phases, colors=colors, alpha=0.85)
    ax2.set_xlabel('frame')
    ax2.set_ylabel('ms')
    ax2.set_title('Stacked frame time')
    ax2.legend(loc='upper right', fontsize=8)

    plt.tight_layout()
    plt.show()

    # ── Summary table ─────────────────────────────────────────────────────────
    print(f'\n{"phase":<12} {"avg ms":>8} {"% of frame":>10}')
    print('-' * 32)
    for p, a in zip(phases, avgs):
        print(f'{p:<12} {a:>8.3f} {a / total_avg * 100:>9.1f}%')
    print('-' * 32)
    print(f'{"total":<12} {total_avg:>8.3f}')
    fps = 1000.0 / total_avg if total_avg > 0 else 0
    bottleneck = phases[avgs.index(max(avgs))]
    print(f'\nbottleneck : {bottleneck}  ({max(avgs):.3f} ms, {max(avgs)/total_avg*100:.1f}% of frame)')
    print(f'effective fps (avg frame time): {fps:.1f}')
    print('\nnote: in CUDA mode, "readback" includes GPU compute + D2H transfer;'
          ' sim/render show only async kernel-launch latency.')


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
    parser.add_argument("--cpu-only",   action="store_true", default=False,
                        help="Build without CUDA (use on machines without NVCC)")
    parser.add_argument("--profiling",  action="store_true", default=False,
                        help="Enable per-frame timing output and plot breakdown")
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
        binary = build(project_root, cpu_only=args.cpu_only, profiling=args.profiling)
    else:
        build_dir = os.path.join(project_root, "build_cpu" if args.cpu_only else "build")
        binary = os.path.join(build_dir, "particles")
        if not os.path.exists(binary):
            sys.exit(f"Binary not found at {binary}. Remove --no-build to compile first.")

    stdout = run_sim(binary, args.particles, args.frames, args.width, args.height,
                     args.dt, args.frames_dir, profiling=args.profiling)

    if args.profiling and stdout:
        plot_profiling(stdout)

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
