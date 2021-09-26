/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <AK/JsonObject.h>
#include <AK/LexicalPath.h>
#include <AK/String.h>
#include <AK/StringBuilder.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/File.h>

class Field
{
public:
    Field(String name, String type) : m_name(move(name)), m_type(move(type)) {}

    const String& name() const { return m_name; }

    const String& type() const { return m_type; }

    bool is_trivial() const { return m_type != "String" && m_type != "UUID"; }

    String create_getter() const
    {
        if (m_type == "Chat::Component")
            return String::formatted("RefPtr<Chat::Component> {}() const {{ return m_{}; }}", m_name, m_name);

        if (is_trivial())
        {
            StringView real_type;

            if (m_type == "VarInt")
                real_type = "i32";
            else if (m_type == "VarLong")
                real_type = "i64";
            else
                real_type = m_type;

            return String::formatted("{} {}() const {{ return m_{}; }}", real_type, m_name, m_name);
        }

        return String::formatted("const {}& {}() const {{ return m_{}; }}", m_type, m_name, m_name);
    }

    String create_setter() const
    {
        if (m_type == "Chat::Component")
            return String::formatted("void set_{}(RefPtr<Chat::Component> value) {{ m_{} = move(value); }}", m_name,
                                     m_name);

        if (is_trivial())
        {
            StringView real_type;

            if (m_type == "VarInt")
                real_type = "i32";
            else if (m_type == "VarLong")
                real_type = "i64";
            else
                real_type = m_type;

            return String::formatted("void set_{}({} value) {{ m_{} = value; }}", m_name, real_type, m_name);
        }

        return String::formatted("void set_{}({} value) {{ m_{} = move(value); }}", m_name, m_type, m_name);
    }

    String create_field() const
    {
        if (m_type == "Chat::Component")
            return String::formatted("RefPtr<Chat::Component> m_{};", m_name);

        if (m_type == "VarInt")
            return String::formatted("i32 m_{}{{}};", m_name);
        else if (m_type == "VarLong")
            return String::formatted("i64 m_{}{{}};", m_name);

        if (is_trivial())
            return String::formatted("BigEndian<{}> m_{}{{}};", m_type, m_name);

        return String::formatted("{} m_{};", m_type, m_name);
    }

    String create_reader() const
    {
        if (m_type == "String")
            return String::formatted("Types::read_string(stream, packet.m_{});", m_name);
        else if (m_type == "VarInt")
            return String::formatted("LEB128::read_signed(stream, packet.m_{});", m_name);
        else if (m_type == "VarLong")
        {
            // TODO: Support reading VarLongs
            TODO();
        }
        else if (m_type == "Chat::Component")
        {
            // TODO: Support reading Chat::Component
            return String::formatted("TODO(); // TODO: Support reading Chat::Component");
        }

        return String::formatted("stream >> packet.m_{};", m_name);
    }

    String create_writer() const
    {
        if (m_type == "String")
            return String::formatted("Types::write_string(stream, m_{});", m_name);
        else if (m_type == "VarInt")
            return String::formatted("Types::write_leb_signed(stream, m_{});", m_name);
        else if (m_type == "VarLong")
        {
            // TODO: Support reading VarLongs
            TODO();
        }
        else if (m_type == "Chat::Component")
            return String::formatted("Types::write_string(stream, m_{}->to_json().to_string());", m_name);

        return String::formatted("stream << m_{};", m_name);
    }

private:
    String m_name;
    String m_type;
};

int main(int argc, char** argv)
{
    Core::ArgsParser args_parser;
    String input_file_path;
    args_parser.add_positional_argument(input_file_path, "Path to the input file", "file");

    if (!args_parser.parse(argc, argv))
        return 1;

    auto file = Core::File::construct(input_file_path);
    if (!file->open(Core::OpenMode::ReadOnly))
    {
        warnln("Failed to open file.");
        return 2;
    }

    auto contents = file->read_all();

    auto json = JsonValue::from_string(contents);
    if (!json.has_value() || !json->is_object())
    {
        warnln("Invalid JSON format");
        return 3;
    }

    auto json_object = json->as_object();

    auto json_fields = json_object.get("fields");
    if (json_fields.type() != AK::JsonValue::Type::Array)
    {
        warnln("No fields.");
        return 5;
    }

    Vector<Field> fields;

    json_fields.as_array().for_each([&](auto& value) {
        if (!value.is_object())
            return;

        auto name = value.as_object().get("name").as_string();
        auto type = value.as_object().get("type").as_string();
        fields.append(Field(move(name), move(type)));
    });

    AK::LexicalPath lexical_path_to_input_file(input_file_path);

    auto class_name = lexical_path_to_input_file.title();

    auto packet_id_namespace = String::formatted("{}::{}", lexical_path_to_input_file.parent().parent().title(),
                                                 lexical_path_to_input_file.parent().title());

    auto packet_id_enum = String::formatted("Id::{}", packet_id_namespace, class_name);

    outln("#pragma once");
    outln();
    outln("#include <AK/Endian.h>");
    outln("#include <AK/LEB128.h>");
    outln("#include <AK/MemoryStream.h>");
    outln("#include <LibMinecraft/Net/Packet.h>");
    outln("#include <LibMinecraft/Net/Types.h>");
    outln("#include <LibMinecraft/Chat/Component.h>");
    outln();
    outln("// This was auto-generated from {}", lexical_path_to_input_file);
    outln("namespace Minecraft::Net::Packets::{}", packet_id_namespace);
    outln("{{");

    outln("class {} : public Packet", class_name);
    outln("{{");

    outln("public:");
    outln("{}() = default;", class_name);
    outln();

    outln("const char* packet_name() const override");
    outln("{{");
    outln("return \"{}::{}\";", packet_id_namespace, class_name);
    outln("}}");

    outln();
    outln("static Optional<{}> from_bytes(InputStream& stream)", class_name);
    outln("{{");

    outln("{} packet;", class_name);
    for (auto& field : fields)
        outln("{}", field.create_reader());

    outln("return packet;");
    outln("}}");

    outln("ByteBuffer to_bytes() const override");
    outln("{{");

    outln("constexpr auto packet_id = Packet::{}::{};", packet_id_enum, class_name);

    outln("DuplexMemoryStream stream;");
    outln("Types::write_leb_signed(stream, static_cast<i32>(packet_id));");
    for (auto& field : fields)
        outln("{}", field.create_writer());

    outln("return stream.copy_into_contiguous_buffer();");

    outln("}}");

    outln();
    for (auto& field : fields)
    {
        outln("{}", field.create_getter());
        outln("{}", field.create_setter());
    }

    outln();
    outln("private:");

    for (auto& field : fields)
        outln("{}", field.create_field());

    outln("}};");
    outln("}}");
}