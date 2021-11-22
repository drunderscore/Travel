/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/Result.h>
#include <AK/String.h>
#include <AK/Vector.h>
#include <LibMinecraft/BlockState.h>
#include <LibMinecraft/NBT/Value.h>

namespace Minecraft
{
class SpongeSchematic
{
public:
    static Result<SpongeSchematic, String> parse_schematic(NBT::Value&);

    i32 data_version() const { return m_data_version; }
    u16 width() const { return m_width; }
    u16 height() const { return m_height; }
    u16 length() const { return m_length; }

    const HashMap<i32, BlockState>& palette() const { return m_palette; }

    const Vector<i32>& block_data() const { return m_block_data; }

    const BlockState& at(u16 x, u16 y, u16 z) const
    {
        auto state = m_palette.find(m_block_data.at(x + z * m_width + y * m_width * m_length));
        VERIFY(state != m_palette.end());
        return state->value;
    }

private:
    SpongeSchematic() = default;

    i32 m_data_version{};
    u16 m_width{};
    u16 m_height{};
    u16 m_length{};
    HashMap<i32, BlockState> m_palette;
    // FIXME: Vectors can hold references, should I use that instead?
    Vector<i32> m_block_data;
    // block data
    // block entities
    // entities
    // biome palette
    // biome data
};
}