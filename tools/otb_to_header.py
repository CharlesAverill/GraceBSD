#!/usr/bin/env python3

import argparse
import re
import struct
import sys
from pathlib import Path

# Pixel values written for each glyph pixel
PIXEL_ON =  "0xFFFFFFFF"
PIXEL_OFF = "0x00000000"

# The header holds one glyph per ASCII code
NUM_CHARS = 128


def read_tables(data: bytes) -> dict:
    """Map each table tag in an OpenType file to its (offset, length)."""
    if len(data) < 12:
        raise ValueError("file too short for an OpenType header")

    num_tables = struct.unpack_from(">H", data, 4)[0]
    tables = {}
    for i in range(num_tables):
        tag, _checksum, offset, length = struct.unpack_from(">4sIII", data, 12 + 16 * i)
        tables[tag.decode("latin-1")] = (offset, length)
    return tables


def read_cmap(data: bytes, tables: dict) -> dict:
    """Map Unicode code points to glyph IDs, from a format 4 cmap subtable."""
    if "cmap" not in tables:
        raise ValueError("font has no cmap table")
    base = tables["cmap"][0]

    num_subtables = struct.unpack_from(">H", data, base + 2)[0]
    subtable = None
    for i in range(num_subtables):
        platform, encoding, offset = struct.unpack_from(">HHI", data, base + 4 + 8 * i)
        fmt = struct.unpack_from(">H", data, base + offset)[0]
        # Windows Unicode BMP, or Unicode BMP
        if fmt == 4 and (platform, encoding) in ((3, 1), (0, 3)):
            subtable = base + offset
            break
    if subtable is None:
        raise ValueError("font has no Unicode format 4 cmap subtable")

    seg_count = struct.unpack_from(">H", data, subtable + 6)[0] // 2
    end_codes = subtable + 14
    start_codes = end_codes + 2 * seg_count + 2
    id_deltas = start_codes + 2 * seg_count
    id_range_offsets = id_deltas + 2 * seg_count

    mapping = {}
    for seg in range(seg_count):
        end = struct.unpack_from(">H", data, end_codes + 2 * seg)[0]
        start = struct.unpack_from(">H", data, start_codes + 2 * seg)[0]
        delta = struct.unpack_from(">h", data, id_deltas + 2 * seg)[0]
        range_offset_pos = id_range_offsets + 2 * seg
        range_offset = struct.unpack_from(">H", data, range_offset_pos)[0]

        for code in range(start, end + 1):
            if code == 0xFFFF:
                continue
            if range_offset == 0:
                glyph = (code + delta) & 0xFFFF
            else:
                glyph_pos = range_offset_pos + range_offset + 2 * (code - start)
                glyph = struct.unpack_from(">H", data, glyph_pos)[0]
                if glyph != 0:
                    glyph = (glyph + delta) & 0xFFFF
            if glyph != 0:
                mapping[code] = glyph
    return mapping


def unpack_small_metrics(data: bytes, pos: int) -> dict:
    height, width, bearing_x, bearing_y, _advance = struct.unpack_from(">BBbbB", data, pos)
    return {"height": height, "width": width, "bearing_x": bearing_x, "bearing_y": bearing_y}


def unpack_big_metrics(data: bytes, pos: int) -> dict:
    # Only the horizontal metrics are needed
    height, width, bearing_x, bearing_y, _advance = struct.unpack_from(">BBbbB", data, pos)
    return {"height": height, "width": width, "bearing_x": bearing_x, "bearing_y": bearing_y}


def read_bits(data: bytes, pos: int, metrics: dict, byte_aligned: bool) -> list:
    """Unpack a 1-bit bitmap into rows of booleans, most significant bit first."""
    width, height = metrics["width"], metrics["height"]
    rows = []
    bit = 0
    for _y in range(height):
        if byte_aligned:
            bit = (bit + 7) // 8 * 8
        row = []
        for _x in range(width):
            byte = data[pos + bit // 8]
            row.append(bool(byte & (0x80 >> (bit % 8))))
            bit += 1
        rows.append(row)
    return rows


def read_glyph_image(data: bytes, pos: int, image_format: int, metrics) -> tuple:
    """Return (metrics, rows) for one glyph image in the EBDT table."""
    if image_format == 1:
        return unpack_small_metrics(data, pos), None, (pos + 5, True)
    if image_format == 2:
        return unpack_small_metrics(data, pos), None, (pos + 5, False)
    if image_format == 5:
        return metrics, None, (pos, False)
    if image_format == 6:
        return unpack_big_metrics(data, pos), None, (pos + 8, True)
    if image_format == 7:
        return unpack_big_metrics(data, pos), None, (pos + 8, False)
    raise ValueError(f"unsupported EBDT image format {image_format}")


def read_glyphs(data: bytes, tables: dict, wanted: set) -> dict:
    """Read the bitmaps of the wanted glyph IDs from the first EBLC strike."""
    for tag in ("EBLC", "EBDT"):
        if tag not in tables:
            raise ValueError(f"font has no {tag} table; is it a bitmap (OTB) font?")
    eblc = tables["EBLC"][0]
    ebdt = tables["EBDT"][0]

    num_sizes = struct.unpack_from(">I", data, eblc + 4)[0]
    if num_sizes < 1:
        raise ValueError("font has no bitmap strikes")
    if num_sizes > 1:
        print(f"otb_to_header.py: warning: using the first of {num_sizes} strikes", file=sys.stderr)

    size = eblc + 8
    array_offset, _table_size, num_subtables = struct.unpack_from(">III", data, size)
    bit_depth = struct.unpack_from(">B", data, size + 46)[0]
    if bit_depth != 1:
        raise ValueError(f"only 1-bit bitmaps are supported, not {bit_depth}-bit")

    glyphs = {}
    for k in range(num_subtables):
        entry = eblc + array_offset + 8 * k
        first, last, additional_offset = struct.unpack_from(">HHI", data, entry)
        sub = eblc + array_offset + additional_offset
        index_format, image_format, image_data_offset = struct.unpack_from(">HHI", data, sub)
        image_base = ebdt + image_data_offset

        for glyph in range(first, last + 1):
            if glyph not in wanted:
                continue
            i = glyph - first
            metrics = None
            if index_format == 1:
                start, end = struct.unpack_from(">II", data, sub + 8 + 4 * i)
            elif index_format == 3:
                start, end = struct.unpack_from(">HH", data, sub + 8 + 2 * i)
            elif index_format == 2:
                image_size = struct.unpack_from(">I", data, sub + 8)[0]
                metrics = unpack_big_metrics(data, sub + 12)
                start, end = i * image_size, (i + 1) * image_size
            else:
                raise ValueError(f"unsupported EBLC index format {index_format}")
            if start == end:
                # No image stored for this glyph
                continue

            metrics, _, (bits_pos, byte_aligned) = read_glyph_image(
                data, image_base + start, image_format, metrics
            )
            glyphs[glyph] = (metrics, read_bits(data, bits_pos, metrics, byte_aligned))
    return glyphs


def font_cell(glyphs: dict) -> tuple:
    """The (width, height, ascent) of a cell that fits every glyph."""
    width = max(m["bearing_x"] + m["width"] for m, _ in glyphs.values())
    ascent = max(m["bearing_y"] for m, _ in glyphs.values())
    descent = max(m["height"] - m["bearing_y"] for m, _ in glyphs.values())
    return width, ascent + descent, ascent


def render_cell(glyph, width: int, height: int, ascent: int) -> list:
    """Place a glyph in a width x height cell, positioned by its metrics."""
    cell = [[False] * width for _ in range(height)]
    if glyph is None:
        return cell
    metrics, rows = glyph
    for y, row in enumerate(rows):
        for x, on in enumerate(row):
            cx = metrics["bearing_x"] + x
            cy = ascent - metrics["bearing_y"] + y
            if on and 0 <= cx < width and 0 <= cy < height:
                cell[cy][cx] = True
    return cell


def char_comment(code: int) -> str:
    if 0x20 < code < 0x7F and chr(code) not in "\\'":
        return f"'{chr(code)}' (0x{code:02X})"
    if code == 0x20:
        return "' ' (0x20)"
    return f"0x{code:02X}"


def convert_font(source: Path, output: Path) -> None:
    data = source.read_bytes()
    tables = read_tables(data)
    cmap = read_cmap(data, tables)

    # Codes the font doesn't map are left blank. Fonts for the IBM PC's code
    # page 437 often map the control codes to its symbols (smileys, arrows).
    glyph_ids = {code: cmap[code] for code in range(NUM_CHARS) if code in cmap}
    glyphs = read_glyphs(data, tables, set(glyph_ids.values()))
    if not glyphs:
        raise ValueError("font has no bitmaps for any ASCII character")

    width, height, ascent = font_cell(glyphs)

    name = re.sub(r"[^0-9a-zA-Z]+", "_", source.stem).strip("_")
    ident = "font_" + name.lower()
    macro = ident.upper()

    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="ascii") as f:
        f.write(f"// Generated from {source.name} by otb_to_header.py. Do not edit.\n\n")
        f.write(f"#ifndef GRACE_{macro}_H\n")
        f.write(f"#define GRACE_{macro}_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define {macro}_WIDTH {width}u\n")
        f.write(f"#define {macro}_HEIGHT {height}u\n\n")
        f.write(f"// Indexed by ASCII code. Each glyph is {macro}_HEIGHT rows of\n")
        f.write(f"// {macro}_WIDTH pixels, stored row by row, so the pixel at\n")
        f.write(f"// (col, row) is [code][row * {macro}_WIDTH + col]. Each pixel is\n")
        f.write(f"// {PIXEL_OFF} (off) or {PIXEL_ON} (on).\n")
        f.write(f"static const uint32_t {ident}[{NUM_CHARS}][{macro}_HEIGHT * {macro}_WIDTH] = {{\n")

        for code in range(NUM_CHARS):
            cell = render_cell(glyphs.get(glyph_ids.get(code)), width, height, ascent)
            f.write(f"    // {char_comment(code)}\n")
            f.write("    {\n")
            for row in cell:
                values = ", ".join(PIXEL_ON if on else PIXEL_OFF for on in row)
                art = "".join("#" if on else "." for on in row)
                f.write(f"        {values}, // {art}\n")
            f.write("    },\n")

        f.write("};\n\n")
        f.write("#endif\n")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Convert a 1-bit OpenType bitmap (OTB) font to a C header of ASCII glyphs."
    )
    parser.add_argument("input", type=Path, help="input OTB font")
    parser.add_argument("output", type=Path, help="output C header")
    args = parser.parse_args()

    try:
        convert_font(args.input, args.output)
    except (OSError, ValueError, struct.error) as exc:
        print(f"otb_to_header.py: error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
