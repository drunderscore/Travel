/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <AK/MemoryStream.h>
#include <LibMinecraft/SpongeSchematic.h>

constexpr i32 sponge_schematic_version = 2;

namespace Minecraft
{
Result<SpongeSchematic, String> SpongeSchematic::parse_schematic(NBT::Value& value)
{
    auto compound = value.as<NBT::Value::Compound*>();

    if (!compound->contains("Version"))
        return {"Missing required \"Version\" field"};

    if (value["Version"].as<BigEndian<i32>>() != sponge_schematic_version)
        return {"Unsupported schematic version"};

    if (!compound->contains("DataVersion"))
        return {"Missing required \"DataVersion\" field"};
    if (!compound->contains("Width"))
        return {"Missing required \"Width\" field"};
    if (!compound->contains("Height"))
        return {"Missing required \"Height\" field"};
    if (!compound->contains("Length"))
        return {"Missing required \"Length\" field"};
    // FIXME: Palette is not required by the spec. If it is not present, values in BlockData are indexes into
    //        the Global Palette (which we don't have a model of yet)
    if (!compound->contains("Palette"))
        return {"Missing required \"Palette\" field"};
    if (!compound->contains("BlockData"))
        return {"Missing required \"BlockData\" field"};

    SpongeSchematic schematic;
    schematic.m_data_version = value["DataVersion"].as<BigEndian<i32>>();
    schematic.m_width = value["Width"].as<BigEndian<i16>>();
    schematic.m_height = value["Height"].as<BigEndian<i16>>();
    schematic.m_length = value["Length"].as<BigEndian<i16>>();

    for (auto& kv : *value["Palette"].as<NBT::Value::Compound*>())
    {
        auto maybe_palette_block_state = BlockState::parse_block_state(kv.key);
        if (maybe_palette_block_state.is_error())
            maybe_palette_block_state.error();

        schematic.m_palette.set(kv.value.as<BigEndian<i32>>(), maybe_palette_block_state.release_value());
    }

    // FIXME: Is this okay to do? (i8 to u8) Not much of a choice, NBT spec and this spec clash horribly...
    auto block_data = value["BlockData"].as<Vector<i8>>();
    auto block_data_bytes = ReadonlyBytes(block_data.data(), block_data.size());
    InputMemoryStream block_data_byte_stream(block_data_bytes);

    while (!block_data_byte_stream.eof())
    {
        i32 palette_index = 0;
        if (!block_data_byte_stream.read_LEB128_signed(palette_index))
            return {"Unable to read LEB128 integer from BlockData"};

        schematic.m_block_data.append(palette_index);
    }

    return schematic;
}
}