/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <LibMinecraft/Chat/Component.h>
#include <LibMinecraft/Net/Packets/Status/Clientbound/Response.h>

typedef struct lua_State lua_State;

namespace Scripting
{
class Types
{
public:
    static void chat_component(lua_State*, const Minecraft::Chat::Component&);

    static NonnullRefPtr<Minecraft::Chat::Component> chat_component(lua_State*, int index);

    static Minecraft::Net::Packets::Status::Clientbound::Response::Data status_request_response_data(lua_State*,
                                                                                                     int index);
};
}