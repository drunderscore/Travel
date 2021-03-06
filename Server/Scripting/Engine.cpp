/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include "Types.h"
#include <AK/Assertions.h>
#include <AK/JsonObject.h>
#include <AK/LexicalPath.h>
#include <LibCore/DirIterator.h>
#include <LibMinecraft/Net/Packets/Play/Clientbound/ChatMessage.h>
#include <LibMinecraft/Net/Packets/Play/Clientbound/PlayerListHeaderAndFooter.h>
#include <Server/Scripting/Engine.h>
#include <Server/Scripting/Format.h>
#include <Server/Scripting/Lua.h>
#include <Server/Server.h>

namespace Scripting
{
HashMap<lua_State*, Engine*> Engine::s_engines;

Engine::Engine(Server& server) : m_server(server)
{
    m_state = luaL_newstate();
    VERIFY(m_state);
    s_engines.set(m_state, this);
    lua_atpanic(m_state, at_panic_thunk);
    luaL_openlibs(m_state);

    static const struct luaL_Reg timer_lib[] = {
        {"create", timer_create_thunk}, {"destroy", timer_destroy_thunk}, {"invoke", timer_invoke_thunk}, {}};

    static const struct luaL_Reg client_lib[] = {
        {"disconnect", client_disconnect_thunk},
        {"sendMessage", client_send_message_thunk},
        {"setPlayerListHeaderAndFooter", client_set_player_list_header_and_footer_thunk},
        {}};

    luaL_newmetatable(m_state, "Server::Client");
    lua_pushstring(m_state, "__index");
    lua_pushvalue(m_state, -2);
    lua_settable(m_state, -3);

    luaL_setfuncs(m_state, client_lib, 0);
    lua_pop(m_state, 1);

    luaL_newmetatable(m_state, "Engine::Timer");
    lua_pushstring(m_state, "__index");
    lua_pushvalue(m_state, -2);
    lua_settable(m_state, -3);

    luaL_setfuncs(m_state, timer_lib, 0);
    lua_pop(m_state, 1);

    luaL_newlib(m_state, timer_lib);
    lua_setglobal(m_state, "Timer");

    lua_pushcfunction(m_state, format_thunk);
    lua_setglobal(m_state, "format");

    auto errored = luaL_dofile(m_state, "Base/Base.lua");
    if (errored)
    {
        warnln("Engine failed startup: {}", lua_tostring(m_state, -1));
        VERIFY_NOT_REACHED();
    }

    m_base_ref = luaL_ref(m_state, LUA_REGISTRYINDEX);

    constexpr StringView plugins_directory = "Plugins";

    if (Core::File::exists(plugins_directory) && Core::File::is_directory(plugins_directory))
    {
        auto plugins_dir_iterator = Core::DirIterator(plugins_directory, Core::DirIterator::SkipDots);

        while (plugins_dir_iterator.has_next())
        {
            auto entry = plugins_dir_iterator.next_full_path();
            if (!Core::File::is_directory(entry))
                continue;

            auto entry_path = LexicalPath(entry);
            auto plugin_main_path = entry_path.append("init.lua");
            if (Core::File::exists(plugin_main_path.string()))
            {
                errored = luaL_dofile(m_state, plugin_main_path.string().characters());
                if (errored)
                {
                    warnln("\u001b[31mFailed to load plugin from path {}\u001b[0m", plugin_main_path);
                    warnln("\u001b[31m{}\u001b[0m", lua_tostring(m_state, -1));
                }
                else
                {
                    outln("\u001b[36mLoaded plugin {}\u001b[0m", plugin_main_path);
                }
            }
        }
    }
    else
    {
        warnln("No plugins directory found, not loading any plugins.");
    }
}

int Engine::format()
{
    lua_pushstring(m_state, Scripting::format(m_state, 1).characters());
    return 1;
}

Engine::~Engine()
{
    s_engines.remove(m_state);
    luaL_unref(m_state, LUA_REGISTRYINDEX, m_base_ref);
    m_base_ref = 0;
    lua_close(m_state);
}

int Engine::at_panic()
{
    warnln("====LUA PANIC!====");
    auto error = lua_tostring(m_state, -1);
    warnln("Why: {}", error);
    luaL_traceback(m_state, m_state, nullptr, 1);
    auto traceback = lua_tostring(m_state, -1);
    warnln("{}", traceback);
    return 0;
}

void Engine::client_did_request_status(Badge<Server>, Client& who)
{
    UsingBaseTable base(*this);
    lua_getfield(m_state, -1, "onRequestStatus");
    client_userdata(who);
    lua_call(m_state, 1, 1);
    auto data = Types::status_request_response_data(m_state, lua_gettop(m_state));
    Minecraft::Net::Packets::Status::Clientbound::Response response(data);
    who.send(response);
}

void Engine::client_did_request_login(Badge<Server>, Client& who,
                                      Minecraft::Net::Packets::Login::Serverbound::LoginStart& packet)
{
    UsingBaseTable base(*this);
    lua_getfield(m_state, -1, "onRequestLogin");
    client_userdata(who);
    lua_pushstring(m_state, packet.username().characters());
    lua_call(m_state, 2, 0);
}

void* Engine::client_userdata(Client& client)
{
    auto client_ud = lua_newuserdata(m_state, sizeof(WeakPtr<Client>));
    new (client_ud) WeakPtr<Client>(client);
    luaL_getmetatable(m_state, "Server::Client");
    lua_setmetatable(m_state, -2);
    return client_ud;
}

void* Engine::timer_userdata(Core::Timer& timer) const
{
    auto* timer_ud = lua_newuserdata(m_state, sizeof(WeakPtr<Core::Timer>));
    new (timer_ud) WeakPtr<Core::Timer>(timer);
    luaL_getmetatable(m_state, "Engine::Timer");
    lua_setmetatable(m_state, -2);
    return timer_ud;
}

int Engine::client_disconnect()
{
    auto client = reinterpret_cast<WeakPtr<Client>*>(luaL_checkudata(m_state, 1, "Server::Client"));
    if (client)
        client->ptr()->disconnect(Types::chat_component(m_state, 2));

    return 0;
}

int Engine::client_send_message()
{
    auto client = reinterpret_cast<WeakPtr<Client>*>(luaL_checkudata(m_state, 1, "Server::Client"));
    if (client)
    {
        auto message = Types::chat_component(m_state, 2);
        auto position = luaL_optinteger(m_state, 3, 0);
        // TODO: Sender UUID
        Minecraft::Net::Packets::Play::Clientbound::ChatMessage chat_message;
        chat_message.set_message(message);
        chat_message.set_position(position);
        client->ptr()->send(chat_message);
    }

    return 0;
}

int Engine::client_set_player_list_header_and_footer()
{
    auto client = reinterpret_cast<WeakPtr<Client>*>(luaL_checkudata(m_state, 1, "Server::Client"));
    if (client)
    {
        RefPtr<Minecraft::Chat::Component> header;
        RefPtr<Minecraft::Chat::Component> footer;

        if(lua_isnil(m_state, 2))
            header = create<Minecraft::Chat::TextComponent>("");
        else
            header = Types::chat_component(m_state, 2);

        if(lua_isnil(m_state, 3))
            footer = create<Minecraft::Chat::TextComponent>("");
        else
            footer = Types::chat_component(m_state, 3);

        Minecraft::Net::Packets::Play::Clientbound::PlayerListHeaderAndFooter player_list_header_and_footer;
        player_list_header_and_footer.set_header(header);
        player_list_header_and_footer.set_footer(footer);
        client->ptr()->send(player_list_header_and_footer);
    }

    return 0;
}

void Engine::push_base_table() const { lua_rawgeti(m_state, LUA_REGISTRYINDEX, m_base_ref); }

int Engine::timer_create()
{
    // Push first function argument to the top of the stack as required by luaL_ref
    lua_pushvalue(m_state, 1);
    auto function_ref = luaL_ref(m_state, LUA_REGISTRYINDEX);
    auto timer = Core::Timer::construct(luaL_checkinteger(m_state, 2), nullptr, nullptr);

    timer->on_timeout = [this, function_ref, timer]() mutable {
        lua_rawgeti(m_state, LUA_REGISTRYINDEX, function_ref);
        lua_call(m_state, 0, 1);
        if (lua_toboolean(m_state, -1))
        {
            luaL_unref(m_state, LUA_REGISTRYINDEX, function_ref);

            timer->stop();
            m_timers.remove_all_matching([timer](auto& other) { return timer.ptr() == other; });
        }
    };

    m_timers.append(timer);

    timer_userdata(timer);

    return 1;
}

int Engine::timer_destroy()
{
    auto timer = reinterpret_cast<WeakPtr<Core::Timer>*>(luaL_checkudata(m_state, 1, "Engine::Timer"));
    if (timer)
        m_timers.remove_all_matching([timer](auto& other) { return timer->ptr() == other; });

    return 0;
}

int Engine::timer_invoke()
{
    auto timer = reinterpret_cast<WeakPtr<Core::Timer>*>(luaL_checkudata(m_state, 1, "Engine::Timer"));
    if (timer)
        timer->ptr()->on_timeout();

    return 0;
}

Engine::UsingBaseTable::~UsingBaseTable() { lua_pop(m_engine.m_state, 1); }

}