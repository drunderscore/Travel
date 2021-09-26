/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/NonnullOwnPtrVector.h>
#include <LibCore/EventLoop.h>
#include <LibCore/Object.h>
#include <LibCore/TCPServer.h>
#include <LibMinecraft/Net/Packets/Login/Serverbound/LoginStart.h>
#include <Server/Client.h>
#include <Server/Scripting/Engine.h>

class Server : public Core::Object
{
    C_OBJECT(Server)
public:
    Server();

    bool listen(IPv4Address addr = {}, u16 port = 25565);

    int exec();

    void client_did_disconnect(Badge<Client>, Client&, Client::DisconnectReason);

    void client_did_request_status(Badge<Client>, Client&);

    void client_did_request_login(Badge<Client>, Client&, Minecraft::Net::Packets::Login::Serverbound::LoginStart&);

private:
    OwnPtr<Scripting::Engine> m_engine;
    NonnullRefPtr<Core::TCPServer> m_server;
    NonnullOwnPtrVector<Client> m_clients;
    Core::EventLoop m_event_loop;
};