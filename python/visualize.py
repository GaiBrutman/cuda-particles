"""
Particle simulation visualizer.

Usage:
    python visualize.py                        # interactive animation
    python visualize.py --output anim.gif      # save GIF
    python visualize.py --output anim.mp4      # save MP4 (requires ffmpeg)
    python visualize.py --frames ./frames --fps 60 --output anim.gif
"""

import argparse
import glob
import os
import sys

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from PIL import Image


def load_frames(frames_dir: str) -> list[np.ndarray]:
    paths = sorted(glob.glob(os.path.join(frames_dir, "frame_*.png")))
    if not paths:
        sys.exit(f"No frame_*.png files found in '{frames_dir}'")
    frames = [np.asarray(Image.open(p).convert("RGBA")) for p in paths]
    print(f"Loaded {len(frames)} frames ({frames[0].shape[1]}×{frames[0].shape[0]})")
    return frames


def build_animation(frames: list[np.ndarray], fps: int) -> animation.FuncAnimation:
    h, w = frames[0].shape[:2]
    fig, ax = plt.subplots(figsize=(w / 100, h / 100), dpi=100)
    fig.patch.set_facecolor("black")
    ax.set_facecolor("black")
    ax.set_xlim(0, w)
    ax.set_ylim(h, 0)  # y-axis: top=0, bottom=h (matches image coords)
    ax.axis("off")
    fig.tight_layout(pad=0)

    im = ax.imshow(frames[0], origin="upper", aspect="equal",
                   extent=[0, w, h, 0], animated=True)
    counter = ax.text(8, 14, "frame 0", color="gray",
                      fontsize=8, fontfamily="monospace", va="top")

    def update(idx: int):
        im.set_data(frames[idx])
        counter.set_text(f"frame {idx:04d}")
        return im, counter

    interval_ms = 1000 / fps
    anim = animation.FuncAnimation(
        fig, update,
        frames=len(frames),
        interval=interval_ms,
        blit=True,
    )
    return fig, anim


def main():
    parser = argparse.ArgumentParser(description="Visualize particle simulation frames")
    parser.add_argument("--frames", default="./frames", help="Directory of frame_*.png files")
    parser.add_argument("--fps",    type=int, default=30, help="Playback / export frame rate")
    parser.add_argument("--output", default=None,
                        help="Save to file instead of interactive display (.gif or .mp4)")
    args = parser.parse_args()

    frames = load_frames(args.frames)
    fig, anim = build_animation(frames, args.fps)

    if args.output:
        ext = os.path.splitext(args.output)[1].lower()
        if ext == ".gif":
            writer = animation.PillowWriter(fps=args.fps)
        elif ext == ".mp4":
            writer = animation.FFMpegWriter(fps=args.fps, bitrate=2000,
                                            extra_args=["-vcodec", "libx264"])
        else:
            sys.exit(f"Unsupported output format '{ext}'. Use .gif or .mp4")

        print(f"Saving {args.output} …")
        anim.save(args.output, writer=writer)
        print("Done.")
    else:
        plt.show()

    plt.close(fig)


if __name__ == "__main__":
    main()
