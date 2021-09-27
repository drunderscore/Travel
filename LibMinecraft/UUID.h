/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/Array.h>
#include <AK/Random.h>
#include <AK/UUID.h>

namespace Minecraft
{
class UUID
{
public:
    UUID() = default;

    UUID generate_random_uuid_v4()
    {
        Array<u8, 16> bytes{};
        AK::fill_with_random(bytes.data(), bytes.size());

        // Logic from OpenJDK:
        // https://hg.openjdk.java.net/jdk8/jdk8/jdk/raw-file/687fd7c7986d/src/share/classes/java/util/UUID.java
        bytes[6] &= 0x0f; // clear version
        bytes[6] |= 0x40; // set to version 4
        bytes[8] &= 0x3f; // clear variant
        bytes[8] |= 0x80; // set to IETF variant

        return {bytes};
    }

    u64 most_significant_bits() const { return m_most_significant_bits; }
    u64 least_significant_bits() const { return m_least_significant_bits; }

    bool operator==(const UUID& other) const
    {
        return m_most_significant_bits == other.m_most_significant_bits &&
               m_least_significant_bits == other.m_least_significant_bits;
    }
    bool operator!=(const UUID& other) const { return !(*this == other); }
    bool operator<=(const UUID&) const = delete;
    bool operator>=(const UUID&) const = delete;
    bool operator<(const UUID&) const = delete;
    bool operator>(const UUID&) const = delete;

    friend InputStream& operator>>(InputStream& stream, Minecraft::UUID& uuid)
    {
        BigEndian<u64> bits;
        stream >> bits;
        uuid.m_most_significant_bits = bits;
        stream >> bits;
        uuid.m_least_significant_bits = bits;

        return stream;
    }

    friend OutputStream& operator<<(OutputStream& stream, const Minecraft::UUID& uuid)
    {
        stream << BigEndian<u64>(uuid.most_significant_bits());
        stream << BigEndian<u64>(uuid.least_significant_bits());

        return stream;
    }

private:
    UUID(ReadonlyBytes bytes)
    {
        VERIFY(bytes.size() == 16);
        for (int i = 0; i < 8; i++)
            m_most_significant_bits = (m_most_significant_bits << 8) | (bytes[i] & 0xff);
        for (int i = 8; i < 16; i++)
            m_least_significant_bits = (m_least_significant_bits << 8) | (bytes[i] & 0xff);
    }

    // Minecraft serializes these over the network as two longs, which we need to do in BigEndian
    // Having this as Array<u8, 16> would make that much harder...
    u64 m_most_significant_bits{};
    u64 m_least_significant_bits{};
};
}