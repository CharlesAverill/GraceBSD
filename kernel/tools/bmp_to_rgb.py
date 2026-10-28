#!/usr/bin/env python3

import argparse
import struct
import sys
from pathlib import Path


def convert_bmp(source: Path, output: Path) -> None:
    data = source.read_bytes()

    if len(data) < 54 or data[:2] != b"BM":
        raise ValueError("not a valid BMP file")

    pixel_offset = struct.unpack_from("<I", data, 10)[0]
    dib_size = struct.unpack_from("<I", data, 14)[0]

    if dib_size < 40 or len(data) < 14 + dib_size:
        raise ValueError("unsupported or truncated BMP header")

    width, signed_height, planes, bpp, compression = struct.unpack_from(
        "<iiHHI", data, 18
    )

    if width <= 0 or signed_height == 0:
        raise ValueError("invalid BMP dimensions")
    if planes != 1:
        raise ValueError("BMP must have exactly one color plane")
    if bpp not in (24, 32):
        raise ValueError("only 24-bit and 32-bit BMP files are supported")
    if compression != 0:
        raise ValueError("only uncompressed BI_RGB BMP files are supported")

    height = abs(signed_height)
    top_down = signed_height < 0
    bytes_per_pixel = bpp // 8

    # BMP rows are padded to a four-byte boundary
    row_stride = (width * bytes_per_pixel + 3) & ~3
    required_size = pixel_offset + row_stride * height

    if required_size > len(data):
        raise ValueError("BMP pixel data is truncated")

    pixels = []

    for y in range(height):
        # Positive height means the BMP stores its rows bottom-up
        source_y = y if top_down else height - 1 - y
        row_start = pixel_offset + source_y * row_stride

        for x in range(width):
            pos = row_start + x * bytes_per_pixel
            b, g, r = data[pos : pos + 3]

            # 0xRRGGBB00
            pixel = (r << 24) | (g << 16) | (b << 8)
            pixels.append(pixel)

    output.parent.mkdir(parents=True, exist_ok=True)

    with output.open("w", encoding="ascii") as f:
        f.write("#ifndef GRACE_SPLASH_DATA_H\n")
        f.write("#define GRACE_SPLASH_DATA_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define SPLASH_WIDTH {width}u\n")
        f.write(f"#define SPLASH_HEIGHT {height}u\n\n")
        f.write(
            "static const uint32_t splash_pixels"
            "[SPLASH_WIDTH * SPLASH_HEIGHT] = {\n"
        )

        for i in range(0, len(pixels), 6):
            chunk = pixels[i : i + 6]
            f.write("    " + ", ".join(f"0x{p:08X}" for p in chunk))
            f.write(",\n" if i + 6 < len(pixels) else "\n")

        f.write("};\n\n")
        f.write("#endif\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert an uncompressed BMP to a 0xRRGGBB00 C pixel array."
    )
    parser.add_argument("input", type=Path, help="input BMP file")
    parser.add_argument("output", type=Path, help="output C header")
    args = parser.parse_args()

    try:
        convert_bmp(args.input, args.output)
    except (OSError, ValueError, struct.error) as exc:
        print(f"bmp_to_rgb.py: error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

