/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/Chat/Component.h>

namespace Minecraft::Chat
{
Component::NamedColorNamesArray Component::s_named_color_names = {
    "black",     "dark_blue", "dark_green", "dark_aqua", "dark_red", "dark_purple",  "gold",   "gray",
    "dark_gray", "blue",      "green",      "aqua",      "red",      "light_purple", "yellow", "white"};

HashMap<String, Component::NamedColor> Component::s_named_color_names_to_value = {
    {"black", NamedColor::Black},
    {"dark_blue", NamedColor::DarkBlue},
    {"dark_green", NamedColor::DarkGreen},
    {"dark_aqua", NamedColor::DarkAqua},
    {"dark_red", NamedColor::DarkRed},
    {"dark_purple", NamedColor::DarkPurple},
    {"gold", NamedColor::Gold},
    {"gray", NamedColor::Gray},
    {"dark_gray", NamedColor::DarkGray},
    {"blue", NamedColor::Blue},
    {"green", NamedColor::Green},
    {"aqua", NamedColor::Aqua},
    {"red", NamedColor::Red},
    {"light_purple", NamedColor::LightPurple},
    {"yellow", NamedColor::Yellow},
    {"white", NamedColor::White},
};

JsonObject Component::to_json() const
{
    JsonObject object;

    if (m_bold.has_value())
        object.set("bold", *m_bold);
    if (m_italic.has_value())
        object.set("italic", *m_italic);
    if (m_underlined.has_value())
        object.set("underlined", *m_underlined);
    if (m_strikethrough.has_value())
        object.set("strikethrough", *m_strikethrough);
    if (m_obfuscated.has_value())
        object.set("obfuscated", *m_obfuscated);
    if (m_font.has_value())
        object.set("font", *m_font);
    if (m_color.has_value())
    {
        m_color->visit(
            [&](NamedColor& named_color) { object.set("color", s_named_color_names[static_cast<i32>(named_color)]); },
            [&](Gfx::Color& color) { object.set("color", color.to_string_without_alpha()); });
    }

    if (!m_children.is_empty())
    {
        JsonArray extra;

        for (auto& child : m_children)
            extra.append(child.to_json());

        object.set("extra", extra);
    }

    return object;
}

void Component::set_color(Optional<NamedColor> value)
{
    if (value.has_value())
        m_color = *value;
    else
        m_color = {};
}

void Component::set_color(Optional<Gfx::Color> value)
{
    if (value.has_value())
        m_color = *value;
    else
        m_color = {};
}

JsonObject TextComponent::to_json() const
{
    auto object = Component::to_json();

    object.set("text", m_text);

    return object;
}

JsonObject TranslationComponent::to_json() const
{
    auto object = Component::to_json();

    object.set("translate", m_translation_key);

    if (!m_translation_format_replacements.is_empty())
    {
        JsonArray format_replacements;

        for (auto& replacement : m_translation_format_replacements)
            format_replacements.append(replacement.to_json());

        object.set("with", format_replacements);
    }

    return object;
}

Result<NonnullRefPtr<Component>, String> Component::parse_component_from_json(const JsonValue& value)
{
    if (!value.is_object())
    {
        // FIXME: Support components that are not objects
        //        It can also be an array of components?
        return String("Value is not a JSON object");
    }

    auto& object = value.as_object();
    RefPtr<Component> the_component;

    if (object.has("text"))
    {
        auto& text_value = object.get("text");
        if (!text_value.is_string())
            return String("\"text\" value is not a string");
        the_component = create<TextComponent>(text_value.as_string());
    }
    else if (object.has("translate"))
    {
        auto& translate_value = object.get("translate");
        if (!translate_value.is_string())
            return String("\"text\" value is not a string");

        auto translation_component = create<TranslationComponent>(translate_value.as_string());

        if (object.has("with"))
        {
            auto& with_value = object.get("with");
            if (!with_value.is_array())
                return String("\"with\" value is not an array");

            for (auto& with_sub_value : with_value.as_array().values())
            {
                auto maybe_component = parse_component_from_json(with_sub_value);
                if (maybe_component.is_error())
                    return String::formatted("Translation format replacement error: {}",
                                             maybe_component.release_error());
                translation_component->append_translation_format_replacement(maybe_component.release_value());
            }
        }

        the_component = translation_component;
    }
    else
    {
        return String("No key exists that represents this component");
    }

    if (object.has("bold"))
    {
        auto& bold_value = object.get("bold");
        if (!bold_value.is_bool())
            return String("\"bold\" value is not a boolean");
        the_component->set_bold(bold_value.as_bool());
    }

    if (object.has("italic"))
    {
        auto& italic_value = object.get("italic");
        if (!italic_value.is_bool())
            return String("\"italic\" value is not a boolean");
        the_component->set_italic(italic_value.as_bool());
    }

    if (object.has("underlined"))
    {
        auto& underlined_value = object.get("underlined");
        if (!underlined_value.is_bool())
            return String("\"underlined\" value is not a boolean");
        the_component->set_underlined(underlined_value.as_bool());
    }

    if (object.has("strikethrough"))
    {
        auto& strikethrough_value = object.get("strikethrough");
        if (!strikethrough_value.is_bool())
            return String("\"strikethrough\" value is not a boolean");
        the_component->set_strikethrough(strikethrough_value.as_bool());
    }

    if (object.has("obfuscated"))
    {
        auto& obfuscated_value = object.get("obfuscated");
        if (!obfuscated_value.is_bool())
            return String("\"obfuscated\" value is not a boolean");
        the_component->set_obfuscated(obfuscated_value.as_bool());
    }

    if (object.has("font"))
    {
        auto& font_value = object.get("font");
        if (!font_value.is_string())
            return String("\"font\" value is not a string");
        the_component->set_font(font_value.as_string());
    }

    if (object.has("color"))
    {
        auto& color_value = object.get("color");
        if (!color_value.is_string())
            return String("\"color\" value is not a string");

        auto color_string = color_value.as_string();
        if (color_string.starts_with('#'))
        {
            auto maybe_color = Gfx::Color::from_string(color_string);
            if (!maybe_color.has_value())
                return String("\"color\" value is not a valid hexadecimal color");
            the_component->set_color(*maybe_color);
        }
        else
        {
            auto maybe_named_color = named_color_value_for_string(color_string);
            if (!maybe_named_color.has_value())
                return String("\"color\" value is not a valid named color");
            the_component->set_color(*maybe_named_color);
        }
    }

    if (object.has("extra"))
    {
        auto& extra_value = object.get("extra");
        if (!extra_value.is_array())
            return String("\"extra\" value is not an array");

        for (auto& extra_sub_value : extra_value.as_array().values())
        {
            auto maybe_component = parse_component_from_json(extra_sub_value);
            if (maybe_component.is_error())
                return maybe_component;

            the_component->append(maybe_component.release_value());
        }
    }

    return the_component.release_nonnull();
}
}
