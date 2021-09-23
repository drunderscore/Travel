/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/ByteBuffer.h>
#include <LibMinecraft/Net/Types.h>

namespace Minecraft::Net
{
class Packet
{
public:
    struct Id
    {
        struct Handshake
        {
            enum class Serverbound
            {
                Handshake
            };
        };

        struct Login
        {
            enum class Clientbound
            {
                Disconnect,
                LoginSuccess = 2
            };

            enum class Serverbound
            {
                LoginStart
            };
        };

        struct Status
        {
            enum class Clientbound
            {
                Response,
                Pong
            };

            enum class Serverbound
            {
                Request,
                Ping
            };
        };

        struct Play
        {
            enum class Clientbound
            {
            };

            enum class Serverbound
            {
            };
        };
    };

    virtual const char* packet_name() const { VERIFY_NOT_REACHED(); }

    virtual ByteBuffer to_bytes() const { VERIFY_NOT_REACHED(); }
};
}