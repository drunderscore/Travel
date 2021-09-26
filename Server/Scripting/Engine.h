/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/Badge.h>
#include <AK/HashMap.h>
#include <AK/RefCounted.h>
#include <LibCore/Timer.h>
#include <Server/Client.h>

typedef struct lua_State lua_State;

#define DEFINE_LUA_METHOD(name)                                                                                        \
    static int name##_thunk(lua_State* state) { return (*s_engines.get(state))->name(); }                              \
    int name();

class Server;

namespace Scripting
{
class Engine
{
public:
    explicit Engine(Server&);

    ~Engine();

    void client_did_request_status(Badge<Server>, Client&);

private:
    static HashMap<lua_State*, Engine*> s_engines;
    lua_State* m_state;
    Server& m_server;
    HashMap<i32, NonnullRefPtr<Core::Timer>> m_timers;
    int m_base_ref{};

    void* timer_userdata(i32 id) const;

    ALWAYS_INLINE void push_base_table() const;

    DEFINE_LUA_METHOD(at_panic);

    DEFINE_LUA_METHOD(format);

    // Timer
    DEFINE_LUA_METHOD(timer_create);

    DEFINE_LUA_METHOD(timer_destroy);

    DEFINE_LUA_METHOD(timer_invoke);

    class UsingBaseTable
    {
        friend Engine;

        UsingBaseTable(const Engine& engine) : m_engine(engine) { engine.push_base_table(); }

        ~UsingBaseTable();

        const Engine& m_engine;
    };
};
}

#undef DEFINE_LUA_METHOD