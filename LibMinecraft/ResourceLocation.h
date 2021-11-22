/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/Format.h>
#include <AK/String.h>
#include <AK/StringView.h>

namespace Minecraft
{
class ResourceLocation
{
public:
    ResourceLocation(String name_space, String path) : m_name_space(move(name_space)), m_path(move(path)) {}

    const String& name_space() const { return m_name_space; }
    const String& path() const { return m_path; }

    String to_string() const { return String::formatted("{}:{}", m_name_space, m_path); }

    static Optional<ResourceLocation> try_parse_from_string(const StringView&);

    struct StandardNamespaces
    {
        static constexpr const char* minecraft = "minecraft";
    };

private:
    String m_name_space;
    String m_path;
};
}

namespace AK
{
template<>
struct Formatter<Minecraft::ResourceLocation> : Formatter<StringView>
{
    void format(FormatBuilder& builder, const Minecraft::ResourceLocation& value)
    {
        Formatter<StringView>::format(builder, value.to_string());
    }
};
}
