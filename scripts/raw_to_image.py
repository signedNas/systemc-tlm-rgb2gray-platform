#!/usr/bin/env python3
"""Convert simulator raw RGB or grayscale output bytes to PNG/JPEG."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


DEFAULT_WIDTH = 1920
DEFAULT_HEIGHT = 1080


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convert headerless raw image bytes from the SystemC RGB-to-gray "
            "platform into a normal image file."
        )
    )
    parser.add_argument("input", type=Path, help="Source .raw file")
    parser.add_argument("output", type=Path, help="Destination image, for example output.png")
    parser.add_argument("--width", type=int, default=DEFAULT_WIDTH, help="Input width in pixels")
    parser.add_argument("--height", type=int, default=DEFAULT_HEIGHT, help="Input height in pixels")
    parser.add_argument(
        "--mode",
        choices=("auto", "rgb", "gray"),
        default="auto",
        help="Raw byte layout. auto infers from file size.",
    )
    parser.add_argument(
        "--quality",
        type=int,
        default=95,
        help="JPEG quality when the output extension is .jpg or .jpeg",
    )
    return parser.parse_args()


def infer_mode(data_len: int, width: int, height: int, requested_mode: str) -> str:
    gray_size = width * height
    rgb_size = gray_size * 3

    if requested_mode == "auto":
        if data_len == gray_size:
            return "gray"
        if data_len == rgb_size:
            return "rgb"
        raise SystemExit(
            f"Cannot infer raw mode from {data_len} bytes. Expected {gray_size} "
            f"bytes for gray or {rgb_size} bytes for RGB."
        )

    expected = rgb_size if requested_mode == "rgb" else gray_size
    if data_len != expected:
        raise SystemExit(
            f"{requested_mode} raw for {width}x{height} must be {expected} bytes, "
            f"but {data_len} bytes were read."
        )
    return requested_mode


def main() -> None:
    args = parse_args()
    raw_bytes = args.input.read_bytes()
    mode = infer_mode(len(raw_bytes), args.width, args.height, args.mode)

    image_mode = "RGB" if mode == "rgb" else "L"
    image = Image.frombytes(image_mode, (args.width, args.height), raw_bytes)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    save_kwargs: dict[str, int] = {}
    if args.output.suffix.lower() in {".jpg", ".jpeg"}:
        save_kwargs["quality"] = args.quality
    image.save(args.output, **save_kwargs)

    print(
        f"Wrote {args.output} from {len(raw_bytes)} bytes "
        f"({args.width}x{args.height}, {mode})"
    )


if __name__ == "__main__":
    main()
