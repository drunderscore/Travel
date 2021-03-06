/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/Net/Packets/Handshake/Serverbound/Handshake.h>
#include <LibMinecraft/Net/Packets/Login/Clientbound/Disconnect.h>
#include <LibMinecraft/Net/Packets/Status/Clientbound/Pong.h>
#include <LibMinecraft/Net/Packets/Status/Clientbound/Response.h>
#include <LibMinecraft/Net/Packets/Status/Serverbound/Ping.h>
#include <LibMinecraft/Net/Types.h>
#include <Server/Client.h>
#include <Server/Server.h>

Client::Client(NonnullRefPtr<Core::TCPSocket> socket, Server& server)
    : m_socket(socket), m_output_stream(socket), m_input_stream(socket), m_server(server)
{
    m_socket->on_ready_to_read = [this]() { on_ready_to_read(); };
}

void Client::send(const Minecraft::Net::Packet& packet)
{
    auto bytes = packet.to_bytes();

    Minecraft::Net::Types::write_leb_signed(m_output_stream, bytes.size());
    m_output_stream << bytes;
}

void Client::forward_raw_bytes(Badge<DestinationServer>, ByteBuffer& bytes) { m_output_stream << bytes; }

void Client::disconnect(Minecraft::Chat::Component& reason)
{
    if (m_current_state == State::Login)
    {
        Minecraft::Net::Packets::Login::Clientbound::Disconnect disconnect;
        disconnect.set_reason(reason);
        send(disconnect);
    }
    else if (m_current_state == State::Play)
    {
        // TODO: Support disconnects during Play state
        VERIFY_NOT_REACHED();
    }
    else
    {
        // Cannot disconnect during this state.
        VERIFY_NOT_REACHED();
    }
    m_server.client_did_disconnect({}, *this, DisconnectReason::DisconnectedByServer);
}

void Client::on_ready_to_read()
{
    if (m_current_destination_server)
    {
        auto bytes = m_socket->read_all();
        m_current_destination_server->forward_raw_bytes({}, bytes);
        return;
    }

    // FIXME: handle TCP disconnects

    // This includes the amount of bytes of the packet id varint
    i32 packet_length;

    if (!LEB128::read_signed(m_input_stream, packet_length))
    {
        warnln("Stream errored trying to read packet size");
        m_input_stream.handle_any_error();
        m_server.client_did_disconnect({}, *this, DisconnectReason::StreamErrored);
        return;
    }

    Minecraft::Net::Types::LEBResult<i32> packet_id;

    if (!Minecraft::Net::Types::read_leb_signed(m_input_stream, packet_id))
    {
        warnln("Stream errored trying to read packet id");
        m_input_stream.handle_any_error();
        m_server.client_did_disconnect({}, *this, DisconnectReason::StreamErrored);
        return;
    }

    auto packet_bytes = m_socket->read(packet_length - packet_id.number_of_bytes_read);

    dbgln("Received ID {} during state {} with {} data bytes", packet_id.value, static_cast<i32>(m_current_state),
          packet_bytes.size());

    switch (m_current_state)
    {
        case State::Handshake:
            handle_handshake_packet(static_cast<Minecraft::Net::Packet::Id::Handshake::Serverbound>(packet_id.value),
                                    packet_bytes);
            break;
        case State::Login:
            handle_login_packet(static_cast<Minecraft::Net::Packet::Id::Login::Serverbound>(packet_id.value),
                                packet_bytes);
            break;
        case State::Status:
            handle_status_packet(static_cast<Minecraft::Net::Packet::Id::Status::Serverbound>(packet_id.value),
                                 packet_bytes);
            break;
        case State::Play:
            handle_play_packet(static_cast<Minecraft::Net::Packet::Id::Play::Serverbound>(packet_id.value),
                               packet_bytes);
            break;
    }
}

void Client::handle_handshake_packet(Minecraft::Net::Packet::Id::Handshake::Serverbound id, ByteBuffer& bytes)
{
    InputMemoryStream stream(bytes);

    if (id == Minecraft::Net::Packet::Id::Handshake::Serverbound::Handshake)
    {
        auto handshake = Minecraft::Net::Packets::Handshake::Serverbound::Handshake::from_bytes(stream);

        // Transition to Status state
        if (handshake->next_state() == 1)
        {
            m_current_state = State::Status;
        }
        // Transition to Login state
        else if (handshake->next_state() == 2)
        {
            m_current_state = State::Login;
        }
        else
        {
            warnln("Client tried to transition into invalid state {}", handshake->next_state());
        }
    }
}

void Client::handle_login_packet(Minecraft::Net::Packet::Id::Login::Serverbound id, ByteBuffer& bytes)
{
    InputMemoryStream stream(bytes);

    if (id == Minecraft::Net::Packet::Id::Login::Serverbound::LoginStart)
    {
        auto login_start = Minecraft::Net::Packets::Login::Serverbound::LoginStart::from_bytes(stream);

        m_server.client_did_request_login({}, *this, *login_start);

        m_current_destination_server = adopt_own(*new DestinationServer(
            DestinationServer::Info({}, 25566, DestinationServer::Info::ConnectionMethod::Unencrypted), *this));
    }
}

void Client::handle_status_packet(Minecraft::Net::Packet::Id::Status::Serverbound id, ByteBuffer& bytes)
{
    InputMemoryStream stream(bytes);

    if (id == Minecraft::Net::Packet::Id::Status::Serverbound::Request)
    {
        m_server.client_did_request_status({}, *this);
    }
    else if (id == Minecraft::Net::Packet::Id::Status::Serverbound::Ping)
    {
        auto ping = Minecraft::Net::Packets::Status::Serverbound::Ping::from_bytes(stream);
        outln("Client is pinging with value {}", ping->value());

        Minecraft::Net::Packets::Status::Clientbound::Pong pong;
        pong.set_value(ping->value());

        send(pong);
    }
}

void Client::handle_play_packet(Minecraft::Net::Packet::Id::Play::Serverbound id, ByteBuffer&) {}