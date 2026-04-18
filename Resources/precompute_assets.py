#!/usr/bin/env python3

from __future__ import annotations

import argparse
import ast
import re
import sys
from pathlib import Path


DIALOGUE_PATTERN = re.compile(r"(?:^|\n)(?:const\s+)?dialogue_t\s+(\w+)\s*=\s*\{", re.MULTILINE)
MAP_PATTERN = re.compile(r"(?:^|\n)(?:const\s+)?map_t\s+(\w+)\s*=\s*\{", re.MULTILINE)
ARRAY_PATTERN = re.compile(
    r"(?:^|\n)(?:uint8_t\s+static\s+const|static\s+const\s+uint8_t)\s+(\w+)\s*(?:\[[^\]]*\])+\s*=\s*\{",
    re.MULTILINE,
)
NUMBER_PATTERN = re.compile(r"0x[0-9A-Fa-f]+|\d+")


def find_matching_brace(text: str, open_index: int) -> int:
    depth = 0
    in_string = False
    escape = False

    for index in range(open_index, len(text)):
        char = text[index]

        if in_string:
            if escape:
                escape = False
            elif char == "\\":
                escape = True
            elif char == '"':
                in_string = False
            continue

        if char == '"':
            in_string = True
        elif char == '{':
            depth += 1
        elif char == '}':
            depth -= 1
            if depth == 0:
                return index

    raise ValueError("unmatched brace while parsing assets.c")


def split_top_level_commas(text: str) -> list[str]:
    parts: list[str] = []
    start = 0
    brace_depth = 0
    bracket_depth = 0
    paren_depth = 0
    in_string = False
    escape = False

    for index, char in enumerate(text):
        if in_string:
            if escape:
                escape = False
            elif char == "\\":
                escape = True
            elif char == '"':
                in_string = False
            continue

        if char == '"':
            in_string = True
        elif char == '{':
            brace_depth += 1
        elif char == '}':
            brace_depth -= 1
        elif char == '[':
            bracket_depth += 1
        elif char == ']':
            bracket_depth -= 1
        elif char == '(':
            paren_depth += 1
        elif char == ')':
            paren_depth -= 1
        elif char == ',' and brace_depth == 0 and bracket_depth == 0 and paren_depth == 0:
            part = text[start:index].strip()
            if part:
                parts.append(part)
            start = index + 1

    tail = text[start:].strip()
    if tail:
        parts.append(tail)

    return parts


def initializer_value(item: str) -> str:
    if '=' in item:
        return item.split('=', 1)[1].strip()
    return item.strip()


def parse_dialogues(source: str) -> list[tuple[str, str]]:
    dialogues: list[tuple[str, str]] = []

    for match in DIALOGUE_PATTERN.finditer(source):
        name = match.group(1)
        open_index = source.find('{', match.end() - 1)
        close_index = find_matching_brace(source, open_index)
        body = source[open_index + 1:close_index]
        items = split_top_level_commas(body)
        if len(items) < 3:
            raise ValueError(f"dialogue '{name}' initializer is incomplete")

        text_token = initializer_value(items[0])
        try:
            text = ast.literal_eval(text_token)
        except Exception as exc:  # pragma: no cover - surfaced to user as build error
            raise ValueError(f"dialogue '{name}' text could not be parsed: {text_token}") from exc

        dialogues.append((name, text))

    return dialogues


def parse_arrays(source: str) -> dict[str, list[int]]:
    arrays: dict[str, list[int]] = {}

    for match in ARRAY_PATTERN.finditer(source):
        name = match.group(1)
        open_index = source.find('{', match.end() - 1)
        close_index = find_matching_brace(source, open_index)
        body = source[open_index + 1:close_index]
        arrays[name] = [int(token, 0) for token in NUMBER_PATTERN.findall(body)]

    return arrays


def parse_maps(source: str) -> list[tuple[str, int, int, str]]:
    maps: list[tuple[str, int, int, str]] = []

    for match in MAP_PATTERN.finditer(source):
        name = match.group(1)
        open_index = source.find('{', match.end() - 1)
        close_index = find_matching_brace(source, open_index)
        body = source[open_index + 1:close_index]
        items = split_top_level_commas(body)
        if len(items) < 3:
            raise ValueError(f"map '{name}' initializer is incomplete")

        width = int(initializer_value(items[0]), 0)
        height = int(initializer_value(items[1]), 0)
        data_symbol = initializer_value(items[2]).lstrip('&')
        maps.append((name, width, height, data_symbol))

    return maps


def compute_dialogue_layout(text: str) -> tuple[list[int], list[list[int]], list[int], list[int]]:
    line_breaks = [0, 0, 0, 0, 0]
    text_origins = [[0, 0] for _ in range(5)]
    line_count = 0
    pos = 0
    max_line_width = 0

    while pos < len(text) and line_count < 5:
        line_start = pos
        line_breaks[line_count] = pos
        last_space = pos
        has_space = False
        char_count = 0

        while pos < len(text) and char_count < 21:
            if text[pos] == ' ':
                last_space = pos
                has_space = True
            pos += 1
            char_count += 1

        if pos >= len(text):
            line_len = pos - line_start
        elif has_space:
            line_len = last_space - line_start
            pos = last_space + 1
        else:
            line_len = 21

        if line_len > 0:
            line_width = 4 * line_len - 1
            if line_width > max_line_width:
                max_line_width = line_width

        line_count += 1

    rectangle_size = [max_line_width + 4, line_count * 6 - 1 + 4 if line_count > 0 else 4]
    rectangle_origin = [(96 - rectangle_size[0]) // 2, 64 - 2 - rectangle_size[1]]

    for index in range(line_count):
        text_origins[index][0] = rectangle_origin[0] + 2
        text_origins[index][1] = rectangle_origin[1] + 2 + index * 6

    return line_breaks, text_origins, rectangle_origin, rectangle_size


def compute_spawn(flattened_map: list[int], width: int, height: int) -> tuple[int, int]:
    for index, value in enumerate(flattened_map):
        if value == 0x20:
            return index // height, index % height

    raise ValueError("spawn tile 0x20 was not found in map data")


def format_u8_list(values: list[int]) -> str:
    return ', '.join(str(value) for value in values)


def format_u8_matrix(values: list[list[int]]) -> str:
    return ', '.join('{' + format_u8_list(row) + '}' for row in values)


def generate_header(source_path: Path, output_path: Path) -> str:
    source = source_path.read_text(encoding='utf-8')
    dialogues = parse_dialogues(source)
    arrays = parse_arrays(source)
    maps = parse_maps(source)

    lines = [
        '/* Auto-generated by Resources/precompute_assets.py. Do not edit by hand. */',
        '#ifndef ASSETS_PRECOMPUTED_H',
        '#define ASSETS_PRECOMPUTED_H',
        '',
    ]

    for name, text in dialogues:
        line_breaks, text_origins, rectangle_origin, rectangle_size = compute_dialogue_layout(text)
        lines.extend(
            [
                f'#define DIALOGUE_LAYOUT_{name} \\',
                f'    .lineBreaks = {{{format_u8_list(line_breaks)}}}, \\',
                f'    .textOrigins = {{{format_u8_matrix(text_origins)}}}, \\',
                f'    .rectangleOrigin = {{{format_u8_list(rectangle_origin)}}}, \\',
                f'    .rectangleSize = {{{format_u8_list(rectangle_size)}}}',
                '',
            ]
        )

    for name, width, height, data_symbol in maps:
        data = arrays.get(data_symbol)
        if data is None:
            raise ValueError(f"map '{name}' references unknown data array '{data_symbol}'")
        spawn_x, spawn_y = compute_spawn(data, width, height)
        lines.extend(
            [
                f'#define MAP_SPAWN_{name} \\',
                f'    .DefaultSpwanPoint = {{{spawn_x}, {spawn_y}}}',
                '',
            ]
        )

    lines.append('#endif')
    lines.append('')

    content = '\n'.join(lines)
    if output_path.exists():
        existing = output_path.read_text(encoding='utf-8')
        if existing == content:
            return content

    output_path.write_text(content, encoding='utf-8')
    return content


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description='Generate precomputed asset initializers from assets.c.')
    parser.add_argument('--source', default='assets.c', help='Path to the source assets.c file')
    parser.add_argument('--output', default='assets_precomputed.h', help='Path to the generated header file')
    args = parser.parse_args(argv)

    source_path = Path(args.source)
    output_path = Path(args.output)

    try:
        generate_header(source_path, output_path)
    except Exception as exc:
        print(f'precompute_assets.py: {exc}', file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))