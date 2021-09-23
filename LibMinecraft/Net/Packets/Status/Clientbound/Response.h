/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/LEB128.h>
#include <AK/MemoryStream.h>
#include <AK/Optional.h>
#include <LibMinecraft/Chat/Component.h>
#include <LibMinecraft/Net/Packet.h>

namespace Minecraft::Net::Packets::Status::Clientbound
{
class Response : public Packet
{
public:
    struct Data
    {
        explicit Data(NonnullRefPtr<Chat::Component> description) : description(move(description)) {}

        struct
        {
            String name;
            int protocol{};
        } version;

        struct
        {
            int max{};
            int online{};

            struct Player
            {
                String name;
                String id;
            };
            Vector<Player> sample;
        } players;

        NonnullRefPtr<Chat::Component> description;
        String favicon;
    };

    explicit Response(Data data) : m_data(move(data)) {}

    ByteBuffer to_bytes() const override;

    static Optional<Response> from_bytes(InputStream&);

    const char* packet_name() const override { return "Status::Clientbound::Response"; }

private:
    Data m_data;
};
}