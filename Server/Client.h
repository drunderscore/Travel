/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/NonnullRefPtr.h>
#include <LibCore/FileStream.h>
#include <LibCore/TCPSocket.h>
#include <LibMinecraft/Chat/Component.h>
#include <LibMinecraft/Net/Packet.h>

class Server;

class Client : public Weakable<Client>
{
public:
    enum class DisconnectReason
    {
        StreamErrored,
        DisconnectedByServer
    };

    enum class State
    {
        Handshake,
        Login,
        Status,
        Play
    };

    Client(NonnullRefPtr<Core::TCPSocket> socket, Server&);

    void send(const Minecraft::Net::Packet&);

    void disconnect(Minecraft::Chat::Component& reason);

private:
    void on_ready_to_read();

    void handle_handshake_packet(Minecraft::Net::Packet::Id::Handshake::Serverbound id, ByteBuffer&);
    void handle_login_packet(Minecraft::Net::Packet::Id::Login::Serverbound id, ByteBuffer&);
    void handle_status_packet(Minecraft::Net::Packet::Id::Status::Serverbound id, ByteBuffer&);
    void handle_play_packet(Minecraft::Net::Packet::Id::Play::Serverbound id, ByteBuffer&);

    State m_current_state{State::Handshake};
    NonnullRefPtr<Core::TCPSocket> m_socket;
    Core::OutputFileStream m_output_stream;
    Core::InputFileStream m_input_stream;
    Server& m_server;
};