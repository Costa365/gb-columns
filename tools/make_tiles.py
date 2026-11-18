"""Encode the original monochrome tile artwork as Game Boy 2bpp."""
import sys
from pathlib import Path


FONT = {
    "A": "0E 11 11 1F 11 11 11", "B": "1E 11 11 1E 11 11 1E",
    "C": "0F 10 10 10 10 10 0F", "D": "1E 11 11 11 11 11 1E",
    "E": "1F 10 10 1E 10 10 1F", "F": "1F 10 10 1E 10 10 10",
    "G": "0F 10 10 17 11 11 0F", "H": "11 11 11 1F 11 11 11",
    "I": "0E 04 04 04 04 04 0E", "J": "07 02 02 02 12 12 0C",
    "K": "11 12 14 18 14 12 11", "L": "10 10 10 10 10 10 1F",
    "M": "11 1B 15 11 11 11 11", "N": "11 19 15 13 11 11 11",
    "O": "0E 11 11 11 11 11 0E", "P": "1E 11 11 1E 10 10 10",
    "Q": "0E 11 11 11 15 12 0D", "R": "1E 11 11 1E 14 12 11",
    "S": "0F 10 10 0E 01 01 1E", "T": "1F 04 04 04 04 04 04",
    "U": "11 11 11 11 11 11 0E", "V": "11 11 11 11 11 0A 04",
    "W": "11 11 11 11 15 1B 11", "X": "11 11 0A 04 0A 11 11",
    "Y": "11 11 0A 04 04 04 04", "Z": "1F 01 02 04 08 10 1F",
    "0": "0E 11 13 15 19 11 0E", "1": "04 0C 04 04 04 04 0E",
    "2": "0E 11 01 02 04 08 1F", "3": "1E 01 01 0E 01 01 1E",
    "4": "02 06 0A 12 1F 02 02", "5": "1F 10 10 1E 01 01 1E",
    "6": "0E 10 10 1E 11 11 0E", "7": "1F 01 02 04 08 08 08",
    "8": "0E 11 11 0E 11 11 0E", "9": "0E 11 11 0F 01 01 0E",
    "-": "00 00 00 1F 00 00 00", "/": "01 01 02 04 08 10 10",
    ".": "00 00 00 00 00 0C 0C", "+": "00 04 04 1F 04 04 00",
    ">": "00 08 0C 0E 0C 08 00", "<": "00 02 06 0E 06 02 00",
    ":": "00 0C 0C 00 0C 0C 00",
}


def canvas(width, height, shade=0):
    return [[shade] * width for _ in range(height)]


def draw_text(pixels, x, y, text, shade, scale):
    for char in text:
        for row, bits in enumerate(FONT[char].split()):
            for col in range(5):
                if int(bits, 16) & (16 >> col):
                    for dy in range(scale):
                        for dx in range(scale):
                            pixels[y + row * scale + dy][x + col * scale + dx] = shade
        x += 6 * scale


SMALL_FONT = {
    "A": "69F99", "D": "E999E", "E": "F8E8F", "H": "99F99",
    "L": "8888F", "M": "9FF99", "N": "9DDB9", "O": "69996",
    "R": "E9EA9", "S": "7861E", "Y": "99622",
}


def draw_small_glyph(pixels, x, char):
    for y, bits in enumerate(SMALL_FONT[char], start=3):
        for col in range(4):
            if int(bits, 16) & (8 >> col):
                pixels[y][x + col] = 0


tiles = []
names = {}


def tile(name, *rows):
    names[name] = len(tiles)
    assert len(rows) == 8 and all(len(row) == 8 for row in rows), name
    tiles.append([[int(pixel) for pixel in row] for row in rows])


tile("EMPTY", "00000000", "00000000", "00000000", "00000000",
     "00000000", "00000000", "00000000", "00000001")
tile("DIAMOND", "00033000", "00333300", "03323330", "33333333",
     "33333333", "03333330", "00333300", "00033000")
tile("RING", "00333300", "03000030", "30000003", "30000003",
     "30000003", "30000003", "03000030", "00333300")
tile("CROSS", "00000000", "00033000", "00033000", "03333330",
     "03333330", "00033000", "00033000", "00000000")
tile("STRIPES", "00000000", "03333330", "01111110", "03333330",
     "01111110", "03333330", "01111110", "00000000")
tile("TRIANGLE", "00000000", "00033000", "00033000", "00333300",
     "00333300", "03333330", "03333330", "00000000")
tile("DOTS", "00000000", "03303300", "03303300", "00000000",
     "03303300", "03303300", "00000000", "00000000")
tile("GHOST", "00000000", "01010100", "00000010", "01000000",
     "00000010", "01000000", "00101010", "00000000")
tile("FLASH", "00020000", "02020200", "00232000", "22333220",
     "00232000", "02020200", "00020000", "00000000")
tile("BLANK", *(["00000000"] * 8))
tile("DARK", *(["33333333"] * 8))
tile("PANEL", *(["11111111"] * 8))
tile("TEXTURE", "22223222", "22232222", "22322222", "23222222",
     "32222223", "22222232", "22222322", "22223222")
tile("RAIL_L", "22223130", "22223130", "22323130", "23223130",
     "32223130", "22223130", "22223130", "22223130")
tile("RAIL_R", "03132222", "03132222", "03132222", "03132223",
     "03132232", "03132322", "03132222", "03132222")
tile("RIM", "33333333", "22222222", "12121212", "00000000",
     "00000000", "00000000", "22222222", "33333333")
tile("RULE", "00000000", "00000000", "00000000", "11111111",
     "22222222", "00000000", "00000000", "00000000")
tile("DIALOG_L", *(["00331000"] * 8))
tile("DIALOG_R", *(["00013300"] * 8))

names["FONT"] = len(tiles)
for code in range(32, 91):
    glyph = canvas(8, 8)
    if chr(code) in FONT:
        draw_text(glyph, 1, 0, chr(code), 3, 1)
    tiles.append(glyph)

# Only the selected difficulty and its arrow use inline inverted text.
inverted_font = [names["DARK"]] * 59
for char in sorted(set("EASY NORMAL HARD>") - {" "}):
    glyph = canvas(8, 8, 3)
    draw_text(glyph, 1, 0, char, 0, 1)
    inverted_font[ord(char) - 32] = len(tiles)
    tiles.append(glyph)

banner = canvas(128, 40)
draw_text(banner, 2, 15, "COLUMNS", 1, 2)
draw_text(banner, 0, 13, "COLUMNS", 3, 2)

# Ore crystal cut into a three-part stack, with a separate contour per piece.
for shift, y0, lower_shade in ((0, 2, 3), (3, 14, 2), (0, 26, 3)):
    for y, (left, right) in enumerate(((9, 30), (7, 33), (5, 35), (3, 38),
                                      (1, 40), (0, 41), (0, 41), (3, 38),
                                      (5, 36), (8, 32), (10, 29), (13, 26))):
        for x in range(left, right + 1):
            if x in (left, right) or y in (0, 11):
                color = 3
            elif y >= 6:
                color = lower_shade if x > 16 else 2
            elif x > 24 + (y // 2):
                color = 2
            elif x < 13 - (y // 2):
                color = 1
            else:
                color = 0
            if (x + y) % 5 == 0 and (x > 29 or y > 7) and color == 2:
                color = 3
            banner[y0 + y][83 + shift + x] = color

for x in range(83, 127):
    banner[39][x] = 1 if x % 3 else 2
names["BANNER"] = len(tiles)
for y in range(0, 40, 8):
    for x in range(0, 128, 8):
        tiles.append([row[x:x + 8] for row in banner[y:y + 8]])

# Match the difficulty order in src/game.c. Right-align each label in four tiles.
names["DIFFICULTY_LABELS"] = len(tiles)
for difficulty in ("EASY", "NORMAL", "HARD"):
    label = canvas(32, 8, 3)
    left = 32 - len(difficulty) * 5
    for i, char in enumerate(difficulty):
        draw_small_glyph(label, left + i * 5, char)
    for x in range(0, 32, 8):
        tiles.append([row[x:x + 8] for row in label])

# One extra pixel above the score text matches the bottom margin of the sidebar.
for name, characters in (("SCORE_LABEL", "SCORE"), ("SCORE_DIGITS", "0123456789")):
    names[name] = len(tiles)
    for char in characters:
        glyph = canvas(8, 8)
        draw_text(glyph, 1, 1, char, 3, 1)
        tiles.append(glyph)

def bar_tilemap(message, bottom, tile_indices):
    # The two tiles of height let 7-pixel glyphs cross the text bar and rim.
    pixels = canvas(160, 16, 3)
    rim = tiles[names["RIM"]]
    for y, row in enumerate(rim[1:4] + rim[5:], start=10):
        pixels[15 - y if bottom else y] = row * 20
    for i, char in enumerate(message, start=1):
        if char != " ":
            draw_text(pixels, i * 8 + 1, 7 if bottom else 2, char, 0, 1)

    indices = []
    for y in (0, 8):
        for x in range(0, 160, 8):
            piece = tuple(tuple(row[x:x + 8]) for row in pixels[y:y + 8])
            if piece not in tile_indices:
                tile_indices[piece] = len(tiles)
                tiles.append(piece)
            indices.append(tile_indices[piece])
    return indices


tile_indices = {tuple(map(tuple, pixels)): i for i, pixels in enumerate(tiles)}
bar_maps = {
    "menu_header": bar_tilemap("CHOOSE DIFFICULTY", False, tile_indices),
    "help_header": bar_tilemap("HOW TO PLAY", False, tile_indices),
    "game_header": bar_tilemap("COLUMNS", False, tile_indices),
    "menu_footer": bar_tilemap("A PLAY      B HELP", True, tile_indices),
    "help_footer": bar_tilemap("A/B BACK", True, tile_indices),
    "game_footer": bar_tilemap("A/B CYCLE  UP DROP", True, tile_indices),
}

assert len(tiles) < 256


def bitplanes(pixels):
    for row in pixels:
        for plane in (0, 1):
            yield sum(((shade >> plane) & 1) << (7 - x) for x, shade in enumerate(row))


definitions = "\n".join(f"#define TILE_{name} {index}" for name, index in names.items())
data = "\n".join("    " + ", ".join(f"0x{byte:02x}" for byte in bitplanes(pixels)) + ","
                 for pixels in tiles)
layouts = "\n".join(
    f"static const unsigned char {name}[] = {{\n    "
    + ", ".join(str(index) for index in indices) + ",\n};"
    for name, indices in {"inverted_font": inverted_font, **bar_maps}.items()
)
Path(sys.argv[1]).write_text(
    f"/* Generated from tools/make_tiles.py. */\n{definitions}\n"
    f"#define TILE_COUNT {len(tiles)}\n"
    f"static const unsigned char tile_data[] = {{\n{data}\n}};\n{layouts}\n"
)
