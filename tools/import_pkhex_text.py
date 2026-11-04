#!/usr/bin/env python3
"""Imports names from PKHeX into strings/.

Usage: import_pkhex_text.py <PKHeX text directory> [strings directory]

The text directory is PKHeX.Core/Resources/text in https://github.com/kwsch/PKHeX (GPLv3).
The strings directory defaults to the one next to this script. It writes:

- species.txt, moves.txt, abilities.txt and items.txt for every language but English, whose
  lists are kept by hand: one name a line, line N naming number N. PKHeX's lists hold the
  names of the latest games, up to Legends: Z-A.
- locationsPLA.txt, locations9.txt and locationsZA.txt for every language: "0xNNNN|name"
  lines with the met and egg locations of Legends: Arceus, Scarlet and Violet, and
  Legends: Z-A. Numbers without a name are left out.

Files are written with CRLF line endings, like the rest of strings/.
"""

import os
import sys

# The strings folder of each language, then PKHeX's language code.
LANGUAGES = [
    ("eng", "en"),
    ("spa", "es"),
    ("fre", "fr"),
    ("ger", "de"),
    ("ita", "it"),
    ("jpn", "ja"),
    ("kor", "ko"),
    ("chs", "zh-Hans"),
    ("cht", "zh-Hant"),
]

# The list in strings/, then PKHeX's file, with {0} for the language code.
NAME_LISTS = [
    ("species.txt", "other/{0}/text_Species_{0}.txt"),
    ("moves.txt", "other/{0}/text_Moves_{0}.txt"),
    ("abilities.txt", "other/{0}/text_Abilities_{0}.txt"),
    ("items.txt", "items/text_Items_{0}.txt"),
]

# The list in strings/, then PKHeX's files: (first number, file), with {0} for the language code.
LOCATION_LISTS = [
    ("locationsPLA.txt", [(0, "locations/gen8a/text_la_00000_{0}.txt"),
                          (30000, "locations/gen8a/text_la_30000_{0}.txt"),
                          (40000, "locations/gen8a/text_la_40000_{0}.txt"),
                          (60000, "locations/gen8a/text_la_60000_{0}.txt")]),
    ("locations9.txt", [(0, "locations/gen9/text_sv_00000_{0}.txt"),
                        (30000, "locations/gen9/text_sv_30000_{0}.txt"),
                        (40000, "locations/gen9/text_sv_40000_{0}.txt"),
                        (60000, "locations/gen9/text_sv_60000_{0}.txt")]),
    ("locationsZA.txt", [(0, "locations/gen9a/text_za_00000_{0}.txt"),
                         (30000, "locations/gen9a/text_za_30000_{0}.txt"),
                         (40000, "locations/gen9a/text_za_40000_{0}.txt"),
                         (60000, "locations/gen9a/text_za_60000_{0}.txt")]),
]


def read_lines(path):
    with open(path, encoding="utf-8-sig") as file:
        lines = file.read().replace("\r\n", "\n").split("\n")
    if lines and lines[-1] == "":
        lines.pop()
    return [line.strip() for line in lines]


def write_text(path, text):
    with open(path, "w", encoding="utf-8", newline="\r\n") as file:
        file.write(text)


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    source = sys.argv[1]
    output = sys.argv[2] if len(sys.argv) == 3 else os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "strings")

    english = {name: read_lines(os.path.join(source, pattern.format("en")))
               for name, pattern in NAME_LISTS}

    for folder, code in LANGUAGES:
        if folder != "eng":
            for name, pattern in NAME_LISTS:
                lines = read_lines(os.path.join(source, pattern.format(code)))
                if len(lines) != len(english[name]):
                    sys.exit(f"{pattern.format(code)} has {len(lines)} names, English "
                             f"{len(english[name])}")
                # The name lists end without a line break.
                write_text(os.path.join(output, folder, name), "\n".join(lines))

        for name, files in LOCATION_LISTS:
            entries = []
            for first, pattern in files:
                for index, line in enumerate(read_lines(os.path.join(source,
                                                                    pattern.format(code)))):
                    if line:
                        entries.append(f"0x{first + index:04x}|{line}")
            write_text(os.path.join(output, folder, name), "\n".join(entries) + "\n")


if __name__ == "__main__":
    main()
