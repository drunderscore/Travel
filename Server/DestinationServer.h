/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/IPv4Address.h>
#include <LibCore/FileStream.h>
#include <LibCore/TCPSocket.h>
#include <LibMinecraft/Net/Packet.h>

class Client;

class DestinationServer
{
public:
    class Info
    {
    public:
        enum class ConnectionMethod
        {
            Unencrypted, // Offline mode
            Velocity,    // Velocity protocol
            BungeeCord   // BungeeCord protocol
        };

        Info(IPv4Address addr, u16 port, ConnectionMethod connection_method)
            : m_address(move(addr)), m_port(port), m_connection_method(connection_method)
        {
        }

        const IPv4Address& address() const { return m_address; }
        u16 port() const { return m_port; }
        ConnectionMethod connection_method() const { return m_connection_method; }

    private:
        IPv4Address m_address;
        u16 m_port;
        ConnectionMethod m_connection_method;
    };

    DestinationServer(Info, Client&);

    const Info& info() const { return m_info; }
    void forward_raw_bytes(Badge<Client>, ByteBuffer&);

private:
    Info m_info;
    Client& m_client;
    NonnullRefPtr<Core::TCPSocket> m_socket;
    Core::OutputFileStream m_output_stream;
    Core::InputFileStream m_input_stream;

    void send(const Minecraft::Net::Packet&);
    void on_connected();
    void on_ready_to_read();
};