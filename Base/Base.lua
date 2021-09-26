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

function Base.onRequestStatus()
    return {description =
{
    text = "I am from ",
    color = "yellow",
    extra = {
        {
            text = "Lua!",
            color = 0xff00ff,
            italic = true
        }
    }
}}
end

return Base
