/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/Endian.h>
#include <AK/HashMap.h>
#include <AK/Result.h>
#include <AK/Stream.h>
#include <AK/String.h>
#include <AK/Variant.h>
#include <AK/Vector.h>

namespace Minecraft::NBT
{
class Value
{
    AK_MAKE_NONCOPYABLE(Value);

public:
    using Compound = HashMap<String, Value>;
    using List = Vector<Value>;

    Value(i8 value) : m_value(value) {}
    Value(BigEndian<i16> value) : m_value(value) {}
    Value(BigEndian<i32> value) : m_value(value) {}
    Value(BigEndian<i64> value) : m_value(value) {}
    Value(BigEndian<float> value) : m_value(value) {}
    Value(BigEndian<double> value) : m_value(value) {}
    Value(Vector<i8> value) : m_value(move(value)) {}
    Value(Vector<i32> value) : m_value(move(value)) {}
    Value(Vector<i64> value) : m_value(move(value)) {}
    Value(String value) : m_value(move(value)) {}

    Value(Compound*&& value) : m_value(move(value)) {}
    Value(List*&& value) : m_value(move(value)) {}

    Value(Value&& other) : m_value(move(other.m_value)) { other.m_value.set<Empty>({}); }

    Value& operator=(Value&& other)
    {
        if (this != &other)
            m_value = move(other.m_value);

        other.m_value.set<Empty>({});

        return *this;
    }

    ~Value()
    {
        if (m_value.has<Compound*>())
            delete m_value.get<Compound*>();
        else if (m_value.has<List*>())
            delete m_value.get<List*>();
    }

    // FIXME: What if we inherited Variant instead?

    const auto& get() const { return m_value; }

    // Let's not allow people to get the Empty, as that's an internal, implementation-level concept :^)
    template<typename T>
    const T& as() const requires(!IsSame<T, Empty>)
    {
        return m_value.get<T>();
    }

    template<typename T>
    T& as() requires(!IsSame<T, Empty>)
    {
        return m_value.get<T>();
    }

    Value& operator[](const String& index)
    {
        // Cannot use get here, it wraps it into an Optional, which passes by value, and we don't want to copy these
        // guys. So instead we do it ourselves.

        // We _could_ return a Value* instead, but that makes chaining []s together hard and annoying with having to
        // deref

        // We _could_ return an Optional<Value*> instead, but it has the same problem as above, with the added bonus of
        // A SECOND deref.

        auto compound = as<Compound*>();
        auto it = compound->find(index);

        VERIFY(it != compound->end());

        return it->value;
    }

    const Value& operator[](const String& index) const
    {
        // Cannot use get here, it wraps it into an Optional, which passes by value, and we don't want to copy these
        // guys. So instead we do it ourselves.

        // We _could_ return a Value* instead, but that makes chaining []s together hard and annoying with having to
        // deref

        // We _could_ return an Optional<Value*> instead, but it has the same problem as above, with the added bonus of
        // A SECOND deref.

        auto compound = as<Compound*>();
        auto it = compound->find(index);

        VERIFY(it != compound->end());

        return it->value;
    }
    
    static Result<Value, String> try_parse(InputStream&);

    enum class Type : i8
    {
        End = 0,
        Byte = 1,
        Short = 2,
        Int = 3,
        Long = 4,
        Float = 5,
        Double = 6,
        ByteArray = 7,
        String = 8,
        List = 9,
        Compound = 10,
        IntArray = 11,
        LongArray = 12
    };

    friend InputStream& operator>>(InputStream& stream, Type& type)
    {
        stream >> *reinterpret_cast<i8*>(&type);
        return stream;
    }

    friend OutputStream& operator<<(OutputStream& stream, const Type& type)
    {
        stream << static_cast<i8>(type);
        return stream;
    }

private:
    // The Empty is used when this Value is moved, so the destructor doesn't kill the just moved contents.
    Variant<i8, BigEndian<i16>, BigEndian<i32>, BigEndian<i64>, BigEndian<float>, BigEndian<double>, Vector<i8>, String,
            List*, Compound*, Vector<i32>, Vector<i64>, Empty>
        m_value;

    static String read_string(InputStream&);
    static Value read_value(Type, InputStream&);

    friend AK::Formatter<Minecraft::NBT::Value>;
};
}
