/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/JsonObject.h>
#include <AK/NonnullRefPtrVector.h>
#include <AK/Optional.h>
#include <AK/RefCounted.h>
#include <AK/Result.h>
#include <AK/String.h>
#include <AK/Variant.h>
#include <LibGfx/Color.h>

namespace Minecraft::Chat
{
class Component : public RefCounted<Component>
{
public:
    enum class NamedColor
    {
        Black,
        DarkBlue,
        DarkGreen,
        DarkAqua,
        DarkRed,
        DarkPurple,
        Gold,
        Gray,
        DarkGray,
        Blue,
        Green,
        Aqua,
        Red,
        LightPurple,
        Yellow,
        White,
        __Count
    };

    Optional<bool> is_bold() const { return m_bold; }
    Optional<bool> is_italic() const { return m_italic; }
    Optional<bool> is_underlined() const { return m_underlined; }
    Optional<bool> has_strikethrough() const { return m_strikethrough; }
    Optional<bool> is_obfuscated() const { return m_obfuscated; }
    const Optional<String>& font() const { return m_font; }
    const Optional<Variant<Gfx::Color, NamedColor>>& color() const { return m_color; }

    void set_bold(Optional<bool> value) { m_bold = move(value); }
    void set_italic(Optional<bool> value) { m_italic = move(value); }
    void set_underlined(Optional<bool> value) { m_underlined = move(value); }
    void set_strikethrough(Optional<bool> value) { m_strikethrough = move(value); }
    void set_obfuscated(Optional<bool> value) { m_obfuscated = move(value); }
    void set_font(Optional<String> value) { m_font = move(value); }
    void set_color(Optional<NamedColor> value);
    void set_color(Optional<Gfx::Color> value);

    const NonnullRefPtrVector<Component>& children() const { return m_children; }

    template<class T, class... Args>
    NonnullRefPtr<T> append(Args&&... args) requires(AK::Detail::IsBaseOf<Component, T>)
    {
        auto component = create<T>(forward<Args>(args)...);
        m_children.append(component);
        return component;
    }
    void append(NonnullRefPtr<Component> component) { m_children.append(component); }

    virtual JsonObject to_json() const;

    static Optional<NamedColor> named_color_value_for_string(const StringView& value)
    {
        return s_named_color_names_to_value.get(value);
    }

    static String named_color_name_for_value(NamedColor color)
    {
        return s_named_color_names[static_cast<i32>(color)];
    }

    static Result<NonnullRefPtr<Component>, String> parse_component_from_json(const JsonValue&);

protected:
    Component() = default;

private:
    using NamedColorNamesArray = Array<String, static_cast<size_t>(NamedColor::__Count)>;
    static NamedColorNamesArray s_named_color_names;
    static HashMap<String, NamedColor> s_named_color_names_to_value;

    Optional<bool> m_bold{};
    Optional<bool> m_italic{};
    Optional<bool> m_underlined{};
    Optional<bool> m_strikethrough{};
    Optional<bool> m_obfuscated{};
    Optional<String> m_font;
    Optional<Variant<Gfx::Color, NamedColor>> m_color;
    // FIXME: Insertion?
    // FIXME: Events

    NonnullRefPtrVector<Component> m_children;
};

class TextComponent : public Component
{
public:
    explicit TextComponent(String text) : m_text(move(text)) {}

    const String& text() const { return m_text; }
    void set_text(String value) { m_text = move(value); }

    JsonObject to_json() const override;

private:
    String m_text;
};

class TranslationComponent : public Component
{
public:
    explicit TranslationComponent(String key) : m_translation_key(move(key)) {}

    const String& translation_key() const { return m_translation_key; }
    void set_translation_key(String value) { m_translation_key = move(value); }

    template<class T, class... Args>
    NonnullRefPtr<T> append_translation_format_replacement(Args&&... args) requires(AK::Detail::IsBaseOf<Component, T>)
    {
        auto component = create<T>(forward<Args>(args)...);
        m_translation_format_replacements.append(component);
        return component;
    }

    void append_translation_format_replacement(NonnullRefPtr<Component> component)
    {
        m_translation_format_replacements.append(component);
    }

    JsonObject to_json() const override;

private:
    String m_translation_key;
    NonnullRefPtrVector<Component> m_translation_format_replacements;
};
}