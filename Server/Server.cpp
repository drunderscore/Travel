/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <Server/Server.h>

Server::Server() : m_server(Core::TCPServer::construct())
{
    m_server->on_ready_to_accept = [this] {
        auto maybe_socket = m_server->accept();
        if (!maybe_socket)
            return;

        m_clients.append(make<Client>(maybe_socket.release_nonnull(), *this));
    };
}

bool Server::listen(IPv4Address addr, u16 port)
{
    if (!m_server->listen(addr, port))
    {
        perror("listen");
        return false;
    }

    return true;
}

int Server::exec() { return m_event_loop.exec(); }

void Server::client_did_disconnect(Badge<Client>, Client& who, Client::DisconnectReason)
{
    deferred_invoke([this, &who](auto&) {
        m_clients.template remove_all_matching([&who](auto& client) { return client.ptr() == &who; });
    });
}