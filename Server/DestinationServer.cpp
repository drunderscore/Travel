/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/Net/Packets/Handshake/Serverbound/Handshake.h>
#include <LibMinecraft/Net/Packets/Login/Serverbound/LoginStart.h>
#include <Server/Client.h>
#include <Server/DestinationServer.h>

DestinationServer::DestinationServer(Info info, Client& client)
    : m_info(move(info)), m_client(client), m_socket(Core::TCPSocket::construct()), m_output_stream(m_socket),
      m_input_stream(m_socket)
{
    m_socket->on_connected = [this]() { on_connected(); };
    m_socket->on_ready_to_read = [this]() { on_ready_to_read(); };

    m_socket->connect(m_info.address(), m_info.port());
}

void DestinationServer::send(const Minecraft::Net::Packet& packet)
{
    auto bytes = packet.to_bytes();

    Minecraft::Net::Types::write_leb_signed(m_output_stream, bytes.size());
    m_output_stream << bytes;
}

void DestinationServer::forward_raw_bytes(Badge<Client>, ByteBuffer& bytes) { m_output_stream << bytes; }

void DestinationServer::on_connected()
{
    // This is all we support right now
    VERIFY(m_info.connection_method() == Info::ConnectionMethod::Unencrypted);

    outln("Connected to destination server, sending handshake and login start");
    Minecraft::Net::Packets::Handshake::Serverbound::Handshake handshake;
    // FIXME: Protocol version constant
    handshake.set_protocol_version(756);
    // FIXME: These are obviously not constant
    handshake.set_server_address("127.0.0.1");
    handshake.set_server_port(25566);
    // FIXME: Magic value of 2
    handshake.set_next_state(2);
    send(handshake);

    Minecraft::Net::Packets::Login::Serverbound::LoginStart login_start;
    login_start.set_username("bro");
    send(login_start);
}

void DestinationServer::on_ready_to_read()
{
    auto bytes = m_socket->read_all();
    outln("Forwarding {} bytes", bytes.size());
    m_client.forward_raw_bytes({}, bytes);
}
