/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once
#include <AK/HashMap.h>
#include <AK/Result.h>
#include <AK/Variant.h>
#include <LibMinecraft/ResourceLocation.h>

namespace Minecraft
{
class BlockState
{
public:
    explicit BlockState(ResourceLocation block) : m_block(move(block)) {}

    void set_state(String& key, String value) { m_states.set(key, move(value)); }

    const ResourceLocation& block() const { return m_block; }
    const HashMap<String, String>& states() const { return m_states; }

    static Result<BlockState, String> parse_block_state(StringView);

    String to_string() const;

private:
    ResourceLocation m_block;
    // TODO: The value here should be a Variant of many types, such as boolean, cardinal direction, integer, etc.
    HashMap<String, String> m_states;
};
}

namespace AK
{
template<>
struct Formatter<Minecraft::BlockState> : Formatter<StringView>
{
    void format(FormatBuilder& builder, const Minecraft::BlockState& value)
    {
        Formatter<StringView>::format(builder, value.to_string());
    }
};
}