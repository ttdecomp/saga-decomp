"""Small, dependency-free ELF32 symbol reader for TU reconstruction."""

from __future__ import annotations

from pathlib import Path
import struct

SHF_ALLOC = 0x2
SHT_SYMTAB = 2
STT_FILE = 4


def workspace_root() -> Path:
    return Path(__file__).resolve().parents[2]


def _cstring(data: bytes, offset: int) -> str:
    end = data.find(b"\0", offset)
    if end == -1:
        end = len(data)
    return data[offset:end].decode("utf-8", errors="replace")


def read_elf32(
    path: Path, *, include_file_symbols: bool = False
) -> tuple[list[dict], list[dict]]:
    """Read named, defined symbols, retaining indices and aliases.

    FILE symbols have special section indices; request them only for
    calibration against a binary that still has those markers.
    """
    data = path.read_bytes()
    if data[:4] != b"\x7fELF" or data[4] != 1:
        raise ValueError(f"{path}: expected a 32-bit ELF file")
    byte_order = {1: "<", 2: ">"}.get(data[5])
    if byte_order is None:
        raise ValueError(f"{path}: unsupported ELF byte order")

    header = struct.unpack_from(byte_order + "HHIIIIIHHHHHH", data, 16)
    section_offset = header[5]
    section_entry_size = header[10]
    section_count = header[11]
    section_names_index = header[12]
    section_struct = struct.Struct(byte_order + "IIIIIIIIII")
    if section_entry_size < section_struct.size:
        raise ValueError(f"{path}: invalid ELF section-header size")

    sections = []
    for index in range(section_count):
        fields = section_struct.unpack_from(
            data, section_offset + index * section_entry_size
        )
        sections.append(
            {
                "index": index,
                "name_offset": fields[0],
                "type": fields[1],
                "flags": fields[2],
                "address": fields[3],
                "offset": fields[4],
                "size": fields[5],
                "link": fields[6],
                "info": fields[7],
                "alignment": fields[8],
                "entry_size": fields[9],
            }
        )

    names_section = sections[section_names_index]
    names = data[
        names_section["offset"] : names_section["offset"] + names_section["size"]
    ]
    for section in sections:
        section["name"] = _cstring(names, section["name_offset"])

    symbols = []
    symbol_struct = struct.Struct(byte_order + "IIIBBH")
    for symbol_table in (section for section in sections if section["type"] == SHT_SYMTAB):
        strings_section = sections[symbol_table["link"]]
        strings = data[
            strings_section["offset"] : strings_section["offset"] + strings_section["size"]
        ]
        entry_size = symbol_table["entry_size"] or symbol_struct.size
        for symbol_index, offset in enumerate(
            range(
                symbol_table["offset"],
                symbol_table["offset"] + symbol_table["size"],
                entry_size,
            )
        ):
            fields = symbol_struct.unpack_from(data, offset)
            section_index = fields[5]
            is_file = (fields[3] & 0x0F) == STT_FILE
            if not fields[0] or (
                section_index >= len(sections)
                and not (include_file_symbols and is_file)
            ):
                continue
            symbols.append(
                {
                    "name": _cstring(strings, fields[0]),
                    "address": fields[1],
                    "size": fields[2],
                    "type": fields[3] & 0x0F,
                    "binding": fields[3] >> 4,
                    "visibility": fields[4] & 0x03,
                    "symbol_index": symbol_index,
                    "symbol_table": symbol_table["name"],
                    "section_index": section_index,
                }
            )
    return sections, symbols
