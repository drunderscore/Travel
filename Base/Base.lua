--[[
    Copyright (c) 2021, James Puleo <james@jame.xyz>

    SPDX-License-Identifier: GPL-3.0-only

    This file belongs to the Base Lua API of Travel.
    You should *NEVER* modify this, or any files around it!
    You will suffer the consequences of me laughing at you
    if you do.
 --]]

print("Travel Base initializing")

package.path = package.path .. ";Base/?.lua"

local Escapes = require("ANSIEscapes")
local Hooks = require("Hooks")

local Base = {}

function Base.onRequestStatus(client)
    local event = {}
    event.client = client
    Hooks.publish("requestStatus", event)
    return event.responseData or {description = {text = "A Minecraft Server"}}
end

function Base.onRequestLogin(client, username)
    local event = {}
    event.client = client
    event.username = username
    Hooks.publish("requestLogin", event)
end

return Base
