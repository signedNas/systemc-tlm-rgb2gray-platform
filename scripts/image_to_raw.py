#!/usr/bin/env python3
"""Convert a PNG/JPEG image to the simulator's raw RGB input format."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


DEFAULT_WIDTH = 1920
DEFAULT_HEIGHT = 1080


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convert PNG/JPEG/etc. to headerless 24-bit RGB raw bytes "
            "for the SystemC RGB-to-gray platform."
        )
    )
    parser.add_argument("input", type=Path, help="Source image, for example pictures/input.jpg")
    parser.add_argument("output", type=Path, help="Destination .raw file")
    parser.add_argument("--width", type=int, default=DEFAULT_WIDTH, help="Output width in pixels")
    parser.add_argument("--height", type=int, default=DEFAULT_HEIGHT, help="Output height in pixels")
    parser.add_argument(
        "--resize",
        action="store_true",
        help="Resize the image to --width x --height instead of rejecting mismatched sizes",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    expected_size = (args.width, args.height)

    with Image.open(args.input) as image:
        if image.size != expected_size:
            if not args.resize:
                raise SystemExit(
                    f"{args.input} is {image.size[0]}x{image.size[1]}, expected "
                    f"{args.width}x{args.height}. Use --resize to scale it."
                )
            image = image.resize(expected_size, Image.Resampling.LANCZOS)

        rgb_image = image.convert("RGB")
        raw_bytes = rgb_image.tobytes()

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(raw_bytes)

    print(
        f"Wrote {len(raw_bytes)} bytes to {args.output} "
        f"({args.width}x{args.height}, RGB888)"
    )


if __name__ == "__main__":
    main()
