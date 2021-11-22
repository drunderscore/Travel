/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <AK/GenericLexer.h>
#include <LibMinecraft/ResourceLocation.h>

namespace Minecraft
{
Optional<ResourceLocation> ResourceLocation::try_parse_from_string(const StringView& maybe_resource_location_string)
{
    // FIXME: Don't allow unacceptable characters
    // FIXME: Correctly support
    GenericLexer lexer(maybe_resource_location_string);
    auto maybe_namespace = lexer.consume_until(':');
    auto maybe_path = lexer.consume_all();

    if (maybe_namespace.is_null() || maybe_path.is_null())
        return {};

    return {{maybe_namespace, maybe_path}};
}
}