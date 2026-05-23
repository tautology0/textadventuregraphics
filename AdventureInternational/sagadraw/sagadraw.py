#!/usr/bin/env python3
"""
sagadraw.py — Extract and render graphics from Scott Adams / SAGA adventure game .SNA files.

Converted from sagadraw.c (originally using the Allegro library) to Python using Pillow.

Usage:
    python sagadraw.py <sna_file> [--game GAME] [--palette PALETTE] [--outdir DIR]

Examples:
    python sagadraw.py adventureland.sna
    python sagadraw.py adventureland.sna --game s1 --palette zx --outdir output_images

Accepted games (--game, default: s1):
    s1      Adventureland
    s3      Secret Mission
    s13     Sorcerer of Claymorgue Castle (ZX)
    s13c64  Sorcerer of Claymorgue Castle (C64)
    q2      Spiderman (ZX)
    q2c64   Spiderman (C64)
    s10     Savage Island 1
    s11     Savage Island 2
    o1      Supergran
    o2      Gremlins

Accepted palettes (--palette, default: zx):
    zx      Authentic Sinclair ZX Spectrum palette
    c64a    Commodore 64 palette, remap A (Adventureland-style, 8-colour games)
    c64b    Commodore 64 palette, remap B (Spiderman-style, 16-colour games)
    vga     Standard VGA palette
"""

import argparse
import json
import os
import sys

from PIL import Image

# ---------------------------------------------------------------------------
# Load configuration from JSON files (match_bytes alongside this script).
# ---------------------------------------------------------------------------

_HERE = os.path.dirname(os.path.abspath(__file__))

def _load_json(filename: str) -> dict:
    path = os.path.join(_HERE, filename)
    if not os.path.isfile(path):
        print(f"Error: configuration file not found: {path}", file=sys.stderr)
        sys.exit(1)
    with open(path) as f:
        return json.load(f)

_palette_config = _load_json("palettes.json")["palettes"]
_game_config    = _load_json("games.json")["games"]

GAMES    = _game_config
PALETTES = _palette_config

INVALID_COLOUR = 16

# Version flags (bitmask)
V_OFFSET_TABLE      = 0x01  # use offset table to locate graphic data
V_RELATIVE_OFFSET   = 0x02  # graphic addresses are relative (TAB_OFFSET is zero)
V_LIMITED_OFFSET    = 0x04  # pointer table immediately precedes graphic data
V_LIMITED_CHARACTER = 0x08  # pointer_table_addr is explicit (not CHAR_START+0x800)
V_HEADER_OFFSETS    = 0x10  # graphic header includes xoff/yoff bytes
V_COLOUR_TYPE       = 0x20  # version 3+ colour encoding (pen bits 0-2, ink bits 3-5)
V_XOFF_ADJUST       = 0x40  # subtract 4 from xoff when compositing


# ---------------------------------------------------------------------------
# Bitmap transform helpers (mirror of the C functions)
# Each character is an 8-element list of bytes, one byte per row.
# ---------------------------------------------------------------------------

def flip_h(char_data: list[int]) -> list[int]:
    """Mirror horizontally (reverse bits in each row byte)."""
    result = []
    for b in char_data:
        rev = 0
        for j in range(8):
            if b & (1 << j):
                rev |= 1 << (7 - j)
        result.append(rev)
    return result


def rot90(char_data: list[int]) -> list[int]:
    """Rotate 90 degrees clockwise."""
    rotated = [0] * 8
    for i in range(8):
        for j in range(8):
            if char_data[j] & (1 << i):
                rotated[7 - i] |= 1 << j
    return rotated


def rot180(char_data: list[int]) -> list[int]:
    """Rotate 180 degrees."""
    rotated = [0] * 8
    for i in range(8):
        for j in range(8):
            if char_data[i] & (1 << j):
                rotated[7 - i] |= 1 << (7 - j)
    return rotated


def rot270(char_data: list[int]) -> list[int]:
    """Rotate 270 degrees clockwise (= 90 counter-clockwise)."""
    rotated = [0] * 8
    for i in range(8):
        for j in range(8):
            if char_data[j] & (1 << i):
                rotated[i] |= 1 << (7 - j)
    return rotated


def transform(sprite_table: list[list[int]], cell_bitmap: list[list[int]], character: int, transform_flags: int, cell_index: int) -> None:
    """
    Apply rotation/flip/mask to sprite[character] and compose onto cell_bitmap[cell_index].

    transform_flags bit layout (matching the original C code):
        bits 4-5 (0x30): rotation  00=none, 01=rot90, 10=rot180, 11=rot270
        bit  6   (0x40): horizontal flip
        bits 2-3 (0x0c): compose mode  00=replace, 01=OR, 10=AND, 11=XOR
    """
    if character == 0xff: return
    rotated = list(sprite_table[character])

    rot = transform_flags & 0x30
    if rot == 0x10:
        rotated = rot90(rotated)
    elif rot == 0x20:
        rotated = rot180(rotated)
    elif rot == 0x30:
        rotated = rot270(rotated)

    if transform_flags & 0x40:
        rotated = flip_h(rotated)

    # The original always applies a final horizontal flip
    rotated = flip_h(rotated)

    compose_mode = transform_flags & 0x0c
    for i in range(8):
        if compose_mode == 0x0c:
            cell_bitmap[cell_index][i] ^= rotated[i]
        elif compose_mode == 0x08:
            cell_bitmap[cell_index][i] &= rotated[i]
        elif compose_mode == 0x04:
            cell_bitmap[cell_index][i] |= rotated[i]
        else:
            cell_bitmap[cell_index][i] = rotated[i]


# ---------------------------------------------------------------------------
# Colour helpers
# ---------------------------------------------------------------------------

def get_palette_rgb(pal_name: str) -> list[tuple[int, int, int]]:
    """Return a list of 17 (R,G,B) tuples for palette indices 0-16 (16=INVALID).

    Palettes with an 'rgb_source' key share their RGB control_byte with another entry
    (e.g. c64a and c64b both point at the 'c64' RGB values).
    """
    entry = PALETTES[pal_name]
    source = entry.get("rgb_source", pal_name)
    raw = PALETTES[source]["rgb"]
    rgb = [(raw[i], raw[i + 1], raw[i + 2]) for i in range(0, 48, 3)]
    rgb.append((255, 0, 255))  # index 16: INVALID (magenta as a sentinel)
    return rgb


def remap_colour(colour: int, pal_name: str) -> int:
    """Remap a raw colour index through the palette's remap table (if any)."""
    if not (0 <= colour <= 15):
        return INVALID_COLOUR
    remap = PALETTES[pal_name].get("remap")
    if remap:
        return remap[colour]
    return colour


# ---------------------------------------------------------------------------
# Main rendering logic
# ---------------------------------------------------------------------------

def render_all(sna_path: str, game_key: str, machine: str, pal_name: str, outdir: str) -> None:
    game    = GAMES[game_key]
    params  = game["machines"][machine]
    sprite_table_addr  = int(params["sprite_table_addr"], 16) if isinstance(params["sprite_table_addr"], str) else params["sprite_table_addr"]
    graphic_count = params["graphic_count"]
    version      = int(params["version"], 16) if isinstance(params["version"], str) else params["version"]
    pointer_base  = int(params["pointer_base"], 16) if isinstance(params["pointer_base"], str) else params["pointer_base"]
    pointer_table_addr  = sprite_table_addr + 0x800 if not (version & V_LIMITED_CHARACTER) else (int(params["pointer_table_addr"], 16) if isinstance(params.get("pointer_table_addr"), str) else params.get("pointer_table_addr"))

    palette_rgb = get_palette_rgb(pal_name)

    os.makedirs(outdir, exist_ok=True)

    with open(sna_path, "rb") as f:
        # --- Read the 255-character sprite table (8 bytes each) ---
        f.seek(sprite_table_addr)
        sprite_table = []
        for _ in range(255):
            sprite_table.append(list(f.read(8)))

        # --- Render each graphic ---
        for graphics in range(graphic_count):
            # Allocate cell_bitmap: up to 600 character cells of 8 bytes each
            cell_bitmap = [[0] * 8 for _ in range(600)]

            # Resolve graphic address
            if version & V_OFFSET_TABLE:
                f.seek(pointer_table_addr + graphics * 2)
                lo = f.read(1)[0]
                hi = f.read(1)[0]
                graphic_addr = lo + hi * 256

                if pointer_base:
                    graphic_addr -= pointer_base
                elif version & V_LIMITED_OFFSET:
                    graphic_addr += pointer_table_addr + (graphic_count * 2)
                else:
                    graphic_addr += pointer_table_addr + 0x100

                print(f"{graphic_addr:x}")
                f.seek(graphic_addr)
            elif graphics == 0:
                f.seek(pointer_base)

            # Read the graphic header
            xsize = f.read(1)[0]
            ysize = f.read(1)[0]
            if version & V_HEADER_OFFSETS:
                xoff = f.read(1)[0]
                yoff = f.read(1)[0]
            else:
                xoff = yoff = 0

            # --- Decode the character stream ---
            cell_index = 0
            while cell_index < xsize * ysize:
                count     = 1
                control_byte      = f.read(1)[0]

                if control_byte < 0x80:
                    # Simple/unmodified character
                    character = control_byte
                    transform(sprite_table, cell_bitmap, character, 0, cell_index)
                    cell_index += 1
                else:
                    # Modified character block
                    if control_byte & 0x02:
                        count = f.read(1)[0] + 1

                    character = f.read(1)[0]
                    if control_byte & 0x01:
                        character += 128 if character < 128 else 0

                    transform_flags = (control_byte & 0xf3) if (control_byte & 0x0c) else control_byte
                    for i in range(count):
                        transform(sprite_table, cell_bitmap, character, transform_flags, cell_index + i)

                    # Handle overlay layers
                    if control_byte & 0x0c:
                        mask_mode = control_byte & 0x0c
                        overlay_byte = f.read(1)[0]
                        prev_control_byte   = control_byte
                        has_more_overlays  = True
                        while has_more_overlays:
                            has_more_overlays = False
                            if overlay_byte < 0x80:
                                for i in range(count):
                                    transform(sprite_table, cell_bitmap, overlay_byte, prev_control_byte & 0x0c, cell_index + i)
                            else:
                                character = f.read(1)[0]
                                if overlay_byte & 0x01:
                                    character += 128 if character < 128 else 0
                                for i in range(count):
                                    transform(sprite_table, cell_bitmap, character,
                                              (overlay_byte & 0xf3) | mask_mode, cell_index + i)
                                if overlay_byte & 0x0c:
                                    mask_mode = overlay_byte & 0x0c
                                    prev_control_byte  = overlay_byte
                                    overlay_byte = f.read(1)[0]
                                    has_more_overlays = True

                    cell_index += count

            # --- Decode the colour stream ---
            fg_colours = [[0] * ysize for _ in range(xsize)]
            bg_colours = [[0] * ysize for _ in range(xsize)]

            col, row   = 0, 0
            colour   = 0

            while row < ysize:
                control_byte = f.read(1)[0]
                if control_byte & 0x80:
                    count = (control_byte & 0x7f) + 1
                    if version & V_COLOUR_TYPE:
                        count -= 1
                    else:
                        colour = f.read(1)[0]
                else:
                    count  = 1
                    colour = control_byte

                while count > 0:
                    if version & V_COLOUR_TYPE:
                        pen_index = colour & 0x07
                        ink_index = (colour & 0x38) >> 3
                        if colour & 0x40:
                            ink_index += 8
                            pen_index += 8
                    else:
                        ink_index = colour & 0x07
                        pen_index = (colour & 0x70) >> 4
                        if colour & 0x08:
                            ink_index += 8
                            pen_index += 8

                    fg_colours[col][row] = pen_index
                    bg_colours[col][row] = ink_index

                    col += 1
                    if col == xsize:
                        col = 0
                        row += 1
                        if row >= ysize:
                            break
                    count -= 1

            # --- Composite the image ---
            # Canvas: 255 × 96 pixels (32 × 12 character cells × 8px each)
            img = Image.new("RGB", (255, 96), (0, 0, 0))
            pixels = img.load()

            cell_index = 0
            for y in range(ysize):
                for x in range(xsize):
                    adjusted_xoff = (xoff - 4) if (version & V_XOFF_ADJUST) else xoff

                    pen_index = fg_colours[x][y]
                    ink_index = bg_colours[x][y]
                    pen_col = palette_rgb[remap_colour(pen_index, pal_name)]
                    ink_col = palette_rgb[remap_colour(ink_index, pal_name)]

                    pixel_x = (x + adjusted_xoff) * 8
                    pixel_y = (y + yoff)  * 8

                    # Fill background (ink)
                    for dy in range(8):
                        for dx in range(8):
                            screen_x, screen_y = pixel_x + dx, pixel_y + dy
                            if 0 <= screen_x < 255 and 0 <= screen_y < 96:
                                pixels[screen_x, screen_y] = ink_col

                    # Draw foreground pixels (pen)
                    char_data = cell_bitmap[cell_index]
                    for row in range(8):
                        for col in range(8):
                            if char_data[row] & (1 << col):
                                screen_x, screen_y = pixel_x + col, pixel_y + row
                                if 0 <= screen_x < 255 and 0 <= screen_y < 96:
                                    pixels[screen_x, screen_y] = pen_col

                    cell_index += 1

            # --- Save ---
            out_filename = f"image{game_key}-{graphics:02d}-{pal_name}.png"
            out_path = os.path.join(outdir, out_filename)
            img.save(out_path)
            print(f"  Saved {out_path}")

    print(f"\nDone. {graphic_count} images written to '{outdir}'.")


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------


def detect_game(sna_path: str, machine: str) -> str | None:
    """Scan games.json entries for a 'strings' list and match against the SNA file.

    Each entry in 'strings' has an 'offset' and a 'string'; all must match for
    the game to be detected. Games with no 'strings' entry are skipped.
    """
    with open(sna_path, "rb") as f:
        for game_key, game in GAMES.items():
            if machine not in game.get("machines", {}):
                continue
            params = game["machines"][machine]
            strings = params.get("strings")
            if strings is None:
                continue
            all_strings_matched = True
            for entry in strings:
                match_offset = entry["offset"]
                match_offset = int(match_offset, 16) if isinstance(match_offset, str) else match_offset
                match_bytes = entry["string"].encode()
                f.seek(match_offset)
                if f.read(len(match_bytes)) != match_bytes:
                    all_strings_matched = False
                    break
            if all_strings_matched:
                return game_key
    return None


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Extract and render graphics from a SAGA adventure game .SNA file.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("sna_file", nargs="?",
                        help="Path to the .SNA snapshot file")
    parser.add_argument("--game",    choices=list(GAMES.keys()), default=None,
                        help="Game identifier (auto-detected if not supplied)")
    parser.add_argument("--machine", choices=["zx", "c64"], default="zx",
                        help="Target machine (default: zx)")
    palette_choices = [ink_index for ink_index, v in PALETTES.items() if "rgb" in v and "rgb_source" not in v or "remap" in v]
    parser.add_argument("--palette", choices=palette_choices, default=None,
                        help="Colour palette to use (default: matches --machine)")
    parser.add_argument("--outdir",  default=None,
                        help="Directory for output PNG files (default: game name)")
    parser.add_argument("--list-games", action="store_true",
                        help="List all available game short names and titles, then exit")

    args = parser.parse_args()

    if args.list_games:
        for key, game in GAMES.items():
            print(f"{key:<10} {game['name']}")
        sys.exit(0)

    if args.palette is None:
        args.palette = args.machine

    if args.sna_file is None:
        parser.error("sna_file is required unless --list-games is used")

    if not os.path.isfile(args.sna_file):
        print(f"Error: file not found: {args.sna_file}", file=sys.stderr)
        sys.exit(1)

    if args.game is None:
        args.game = detect_game(args.sna_file, args.machine)
        if args.game is None:
            print("Cannot detect game, please supply it using the --game option", file=sys.stderr)
            sys.exit(1)

    game_info = GAMES[args.game]
    if args.outdir is None:
        args.outdir = game_info["name"]

    if args.machine not in game_info["machines"]:
        available = ", ".join(game_info["machines"].keys())
        print(f"Error: '{args.game}' has no {args.machine} version (available: {available})", file=sys.stderr)
        sys.exit(1)

    print(f"Game   : {game_info['name']} ({args.machine.upper()})")
    print(f"Palette: {args.palette}")
    print(f"Output : {args.outdir}/")
    print()

    render_all(args.sna_file, args.game, args.machine, args.palette, args.outdir)


if __name__ == "__main__":
    main()
