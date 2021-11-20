/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#include <LibMinecraft/NBT/Value.h>

namespace Minecraft::NBT
{
String Value::read_string(InputStream& stream)
{
    BigEndian<u16> length;
    stream >> length;
    char* value_buffer;
    auto value_string_impl = StringImpl::create_uninitialized(length, value_buffer);
    stream.read({value_buffer, length});
    return value_string_impl;
}

Value Value::read_value(Type tag, InputStream& stream)
{
    switch (tag)
    {
        case Type::Byte:
        {
            i8 value;
            stream >> value;
            return value;
        }
        case Type::Short:
        {
            BigEndian<i16> value;
            stream >> value;
            return value;
        }
        case Type::Int:
        {
            BigEndian<i32> value;
            stream >> value;
            return value;
        }
        case Type::Long:
        {
            BigEndian<i64> value;
            stream >> value;
            return value;
        }
        case Type::Float:
        {
            BigEndian<float> value;
            stream >> value;
            return value;
        }
        case Type::Double:
        {
            BigEndian<double> value;
            stream >> value;
            return value;
        }
        case Type::ByteArray:
        {
            BigEndian<i32> length;
            stream >> length;

            Vector<i8> values;
            values.resize(length);
            i8 individual_value;

            for (auto i = 0; i < length; i++)
            {
                stream >> individual_value;
                values[i] = individual_value;
            }

            return values;
        }
        case Type::String:
            return read_string(stream);
        case Type::Compound:
        {
            auto compound = new Compound;
            Type nested_tag;

            stream >> nested_tag;

            while (nested_tag != Type::End)
            {
                auto name = read_string(stream);
                auto value = read_value(nested_tag, stream);
                compound->set(name, move(value));
                stream >> nested_tag;
            }

            return compound;
        }
        case Type::IntArray:
        {
            BigEndian<i32> length;
            stream >> length;

            Vector<i32> values;
            values.resize(length);
            BigEndian<i32> individual_value;

            for (auto i = 0; i < length; i++)
            {
                stream >> individual_value;
                values[i] = individual_value;
            }

            return values;
        }
        case Type::LongArray:
        {
            BigEndian<i32> length;
            stream >> length;

            Vector<i64> values;
            values.resize(length);
            BigEndian<i64> individual_value;

            for (auto i = 0; i < length; i++)
            {
                stream >> individual_value;
                values[i] = individual_value;
            }

            return values;
        }
        case Type::List:
        {
            Type list_of_this_tag;
            BigEndian<i32> length;
            stream >> list_of_this_tag;
            stream >> length;

            auto values = new Vector<Value>;

            for (auto i = 0; i < length; i++)
                values->append(read_value(list_of_this_tag, stream));

            return values;
        }
        default:
            VERIFY_NOT_REACHED();
    }
}

Result<Value, String> Value::try_parse(InputStream& stream)
{
    // Unclear specification: The original specification just says "byte". In Java, this is signed, so let's
    // do the same.
    Type tag;

    stream >> tag;

    if (tag != Type::Compound)
        return {"NBT does not start with Compound"};

    // FIXME: We have to discard the root compound name... should we care?
    read_string(stream);

    return read_value(tag, stream);
}
}