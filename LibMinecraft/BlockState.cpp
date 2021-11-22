/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/BlockState.h>

namespace Minecraft
{
Result<BlockState, String> BlockState::parse_block_state(StringView value)
{
    GenericLexer lexer(value);
    auto maybe_block_location = ResourceLocation::try_parse_from_string(lexer.consume_until('['));
    if (!maybe_block_location.has_value())
        return {"Invalid block Resource Location"};

    BlockState state(move(*maybe_block_location));

    while (!lexer.is_eof())
    {
        auto key = lexer.consume_until('=').to_string();
        auto value = lexer.consume_until(',');
        if (value.ends_with(']'))
            value = value.substring_view(0, value.length() - 1);

        state.set_state(key, value);
    }

    return state;
}

String BlockState::to_string() const
{
    StringBuilder state_builder;
    state_builder.append(m_block.to_string());

    if (m_states.size() > 0)
    {
        state_builder.append('[');

        int i = 0;

        for (auto& kv : m_states)
        {
            state_builder.append(kv.key);
            state_builder.append('=');
            state_builder.append(kv.value);

            if (i < m_states.size() - 1)
                state_builder.append(',');

            i++;
        }

        state_builder.append(']');
    }

    return state_builder.build();
}
}