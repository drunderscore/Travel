/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/Net/Packets/Status/Clientbound/Response.h>
#include <Server/Scripting/Lua.h>
#include <Server/Scripting/Types.h>

namespace Scripting
{
void Types::chat_component(lua_State* state, const Minecraft::Chat::Component& component)
{
    lua_newtable(state);
    if (component.is_bold().has_value())
    {
        lua_pushstring(state, "bold");
        lua_pushboolean(state, *component.is_bold());
        lua_settable(state, -3);
    }

    if (component.is_italic().has_value())
    {
        lua_pushstring(state, "italic");
        lua_pushboolean(state, *component.is_italic());
        lua_settable(state, -3);
    }

    if (component.is_underlined().has_value())
    {
        lua_pushstring(state, "underlined");
        lua_pushboolean(state, *component.is_underlined());
        lua_settable(state, -3);
    }

    if (component.has_strikethrough().has_value())
    {
        lua_pushstring(state, "strikethrough");
        lua_pushboolean(state, *component.has_strikethrough());
        lua_settable(state, -3);
    }

    if (component.is_obfuscated().has_value())
    {
        lua_pushstring(state, "obfuscated");
        lua_pushboolean(state, *component.is_obfuscated());
        lua_settable(state, -3);
    }

    if (component.font().has_value())
    {
        lua_pushstring(state, "font");
        lua_pushstring(state, component.font()->characters());
        lua_settable(state, -3);
    }

    if (component.color().has_value())
    {
        lua_pushstring(state, "color");
        component.color()->visit(
            [&](Minecraft::Chat::Component::NamedColor& named_color) {
                lua_pushstring(state, Minecraft::Chat::Component::named_color_name_for_value(named_color).characters());
            },
            [&](Gfx::Color& color) { lua_pushinteger(state, color.value()); });
        lua_settable(state, -3);
    }

    if (auto* text_component = dynamic_cast<const Minecraft::Chat::TextComponent*>(&component))
    {
        lua_pushstring(state, "text");
        lua_pushstring(state, text_component->text().characters());
        lua_settable(state, -3);
    }
    else if (auto* translation_component = dynamic_cast<const Minecraft::Chat::TranslationComponent*>(&component))
    {
        lua_pushstring(state, "translate");
        lua_pushstring(state, translation_component->translation_key().characters());
        lua_settable(state, -3);
    }
    else
    {
        VERIFY_NOT_REACHED();
    }
}

NonnullRefPtr<Minecraft::Chat::Component> Types::chat_component(lua_State* state, int index)
{
    auto parse_base_component_values = [state, index](Minecraft::Chat::Component& component) {
        lua_pushstring(state, "bold");
        lua_gettable(state, index);
        if (lua_isboolean(state, -1))
            component.set_bold(lua_toboolean(state, -1));
        lua_pop(state, 1);

        lua_pushstring(state, "italic");
        lua_gettable(state, index);
        if (lua_isboolean(state, -1))
            component.set_italic(lua_toboolean(state, -1));
        lua_pop(state, 1);

        lua_pushstring(state, "underlined");
        lua_gettable(state, index);
        if (lua_isboolean(state, -1))
            component.set_underlined(lua_toboolean(state, -1));
        lua_pop(state, 1);

        lua_pushstring(state, "strikethrough");
        lua_gettable(state, index);
        if (lua_isboolean(state, -1))
            component.set_strikethrough(lua_toboolean(state, -1));
        lua_pop(state, 1);

        lua_pushstring(state, "obfuscated");
        lua_gettable(state, index);
        if (lua_isboolean(state, -1))
            component.set_obfuscated(lua_toboolean(state, -1));
        lua_pop(state, 1);

        lua_pushstring(state, "font");
        lua_gettable(state, index);
        if (lua_isstring(state, -1))
            component.set_font(lua_tostring(state, -1));
        lua_pop(state, 1);

        lua_pushstring(state, "color");
        lua_gettable(state, index);
        if (lua_type(state, -1) == LUA_TSTRING)
        {
            auto color_name = lua_tostring(state, -1);
            auto maybe_named_color = Minecraft::Chat::Component::named_color_value_for_string(color_name);
            if (!maybe_named_color.has_value())
                luaL_error(state, "\"%s\" is not a valid named color", color_name);

            component.set_color(*maybe_named_color);
        }
        else if (lua_isinteger(state, -1))
        {
            component.set_color(Gfx::Color::from_rgb(lua_tointeger(state, -1)));
        }
        lua_pop(state, 1);

        lua_pushstring(state, "extra");
        lua_gettable(state, index);
        auto extra_index = lua_gettop(state);
        if (lua_istable(state, -1))
        {
            lua_pushnil(state);
            while (lua_next(state, extra_index) != 0)
            {
                if (!lua_istable(state, -1))
                    luaL_error(state, "\"extra\" components must be a table");

                component.append(chat_component(state, lua_gettop(state)));

                lua_pop(state, 1);
            }
        }
        lua_pop(state, 1);

        return true;
    };

    lua_pushstring(state, "text");
    lua_gettable(state, index);
    if (lua_isstring(state, -1))
    {
        auto text_component = create<Minecraft::Chat::TextComponent>(lua_tostring(state, -1));
        parse_base_component_values(text_component);

        lua_pop(state, 1);

        return text_component;
    }
    else
    {
        lua_pushstring(state, "translate");
        lua_gettable(state, index);

        if (lua_isstring(state, -1))
        {
            auto translation_component = create<Minecraft::Chat::TranslationComponent>(lua_tostring(state, -1));
            parse_base_component_values(translation_component);

            lua_pop(state, 1);

            return translation_component;
        }
        else
        {
            luaL_error(state, "cannot create chat component without \"text\" or \"translation\" value");
            VERIFY_NOT_REACHED();
        }
    }
}

Minecraft::Net::Packets::Status::Clientbound::Response::Data Types::status_request_response_data(lua_State* state,
                                                                                                 int index)
{
    luaL_checktype(state, index, LUA_TTABLE);

    lua_pushstring(state, "description");
    lua_gettable(state, index);
    auto description = chat_component(state, lua_gettop(state));
    lua_pop(state, 1);

    Minecraft::Net::Packets::Status::Clientbound::Response::Data data(description);
    // FIXME: This is NOT where a constant like this should live.
    data.version.protocol = 756;
    data.version.name = "1.17.1";

    // FIXME: There are more fields to parse here

    return data;
}
}