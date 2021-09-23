/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/Net/Packets/Status/Clientbound/Response.h>

namespace Minecraft::Net::Packets::Status::Clientbound
{
ByteBuffer Response::to_bytes() const
{
    DuplexMemoryStream stream;

    Types::write_leb_signed(stream, static_cast<i32>(Id::Status::Clientbound::Response));

    JsonObject object;
    JsonObject version;
    JsonObject players;

    version.set("name", m_data.version.name);
    version.set("protocol", m_data.version.protocol);

    players.set("max", m_data.players.max);
    players.set("online", m_data.players.online);
    if (!m_data.players.sample.is_empty())
    {
        JsonArray sample;

        for (auto& player : m_data.players.sample)
        {
            JsonObject sample_player;
            sample_player.set("name", player.name);
            sample_player.set("id", player.id);
            sample.append(sample_player);
        }
        players.set("sample", players);
    }

    object.set("version", version);
    object.set("players", players);
    object.set("description", m_data.description->to_json());

    if (!m_data.favicon.is_null())
        object.set("favicon", m_data.favicon);

    Types::write_string(stream, object.to_string());

    return stream.copy_into_contiguous_buffer();
}
}