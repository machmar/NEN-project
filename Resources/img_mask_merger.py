#!/usr/bin/env python3
from __future__ import annotations

import math
import re
import sys


COMMENT_RE = re.compile(
    r"^\s*//\s*'(?P<name>[^']+)'\s*,\s*(?P<w>\d+)x(?P<h>\d+)px\s*$"
)
HEX_RE = re.compile(r"0x[0-9a-fA-F]+|\b\d+\b")


def parse_sections(text: str) -> list[dict]:
    lines = text.splitlines()
    sections = []
    current = None

    for line_no, line in enumerate(lines, start=1):
        m = COMMENT_RE.match(line)
        if m:
            if current is not None:
                sections.append(current)

            current = {
                "name": m.group("name"),
                "width": int(m.group("w")),
                "height": int(m.group("h")),
                "data_tokens": [],
                "line_no": line_no,
            }
            continue

        if current is None:
            continue

        data_part = line.split("//", 1)[0]
        current["data_tokens"].extend(HEX_RE.findall(data_part))

    if current is not None:
        sections.append(current)

    if len(sections) < 2:
        raise ValueError("Expected at least two sections in the pasted text.")

    parsed = []
    for sec in sections:
        values = []
        for tok in sec["data_tokens"]:
            v = int(tok, 0)
            if not 0 <= v <= 0xFF:
                raise ValueError(
                    f"Value out of byte range in section '{sec['name']}': {tok}"
                )
            values.append(v)

        parsed.append({
            "name": sec["name"],
            "width": sec["width"],
            "height": sec["height"],
            "data": values,
        })

    return parsed


def expected_byte_count(width: int, height: int) -> int:
    return width * math.ceil(height / 8)


def combine_interleaved(color: list[int], mask: list[int]) -> list[int]:
    if len(color) != len(mask):
        raise ValueError(
            f"Array size mismatch: color={len(color)} bytes, mask={len(mask)} bytes"
        )

    out = []
    for c, m in zip(color, mask):
        out.append(m)
        out.append(c)
    return out


def infer_base_name(color_name: str, mask_name: str) -> str:
    c = re.sub(r"[-_]?color$", "", color_name, flags=re.IGNORECASE)
    m = re.sub(r"[-_]?mask$", "", mask_name, flags=re.IGNORECASE)
    return c if c == m else f"{c}_{m}"


def format_c_array(name: str, data: list[int], bytes_per_line: int = 16) -> str:
    lines = [f"static const uint8_t {name}[] = {{"]

    for i in range(0, len(data), bytes_per_line):
        chunk = data[i:i + bytes_per_line]
        line = ", ".join(f"0x{b:02x}" for b in chunk)
        if i + bytes_per_line < len(data):
            line += ","
        lines.append(f"    {line}")

    lines.append("};")
    return "\n".join(lines)


def read_pasted_input() -> str:
    print("Paste the two arrays, then finish with:")
    print("  Linux/macOS: Ctrl+D")
    print("  Windows:     Ctrl+Z then Enter")
    print()
    return sys.stdin.read()


def main() -> int:
    try:
        text = read_pasted_input()
        sections = parse_sections(text)

        color_sec = sections[0]
        mask_sec = sections[1]

        if color_sec["width"] != mask_sec["width"] or color_sec["height"] != mask_sec["height"]:
            raise ValueError(
                f"Dimension mismatch:\n"
                f"  {color_sec['name']}: {color_sec['width']}x{color_sec['height']}\n"
                f"  {mask_sec['name']}: {mask_sec['width']}x{mask_sec['height']}"
            )

        expected = expected_byte_count(color_sec["width"], color_sec["height"])

        if len(color_sec["data"]) != expected:
            raise ValueError(
                f"Section '{color_sec['name']}' has {len(color_sec['data'])} bytes, "
                f"but expected {expected} for {color_sec['width']}x{color_sec['height']}."
            )

        if len(mask_sec["data"]) != expected:
            raise ValueError(
                f"Section '{mask_sec['name']}' has {len(mask_sec['data'])} bytes, "
                f"but expected {expected} for {mask_sec['width']}x{mask_sec['height']}."
            )

        combined = combine_interleaved(color_sec["data"], mask_sec["data"])
        array_name = infer_base_name(color_sec["name"], mask_sec["name"]) + "_combined"

        output_text = (
            f"// Combined from '{color_sec['name']}' and '{mask_sec['name']}'\n"
            f"// Dimensions: {color_sec['width']}x{color_sec['height']} px\n"
            f"// Source bytes per array: {expected}\n"
            f"// Output bytes: {len(combined)}\n\n"
            f"{format_c_array(array_name, combined)}\n"
        )

        print("\n" + output_text)
        return 0

    except Exception as e:
        print(f"\nError: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())