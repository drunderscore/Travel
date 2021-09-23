/*
 * Copyright (c) 2021, James Puleo <james@jame.xyz>
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */

#pragma once

#include <AK/Stream.h>
#include <AK/String.h>
#include <AK/Types.h>

namespace Minecraft::Net
{
class Types
{
public:
    template<typename ValueType = size_t>
    struct LEBResult
    {
        ValueType value{};
        size_t number_of_bytes_read{};
    };

    // This is copied from serenity/AK/LEB128.h
    // We need this because the protocol needs to know how many bytes were read for the LEB128 in certain cases.
    template<typename StreamT, typename ValueType = ssize_t>
    static bool read_leb_signed(StreamT& stream, LEBResult<ValueType>& result)
    {
        // Note: We read into a u64 to simplify the parsing logic;
        //    result is range checked into ValueType after parsing.
        static_assert(sizeof(ValueType) <= sizeof(u64), "Error checking logic assumes 64 bits or less!");
        [[maybe_unused]] size_t backup_offset = 0;
        if constexpr (requires { stream.offset(); })
            backup_offset = stream.offset();
        InputStream& input_stream{stream};

        result = {};

        i64 temp = 0;
        u8 byte = 0;

        do
        {
            if (input_stream.unreliable_eof())
            {
                if constexpr (requires { stream.seek(backup_offset); })
                    stream.seek(backup_offset);
                input_stream.set_fatal_error();
                return false;
            }

            input_stream >> byte;
            if (input_stream.has_any_error())
                return false;

            // note: 64 bit assumptions!
            u64 masked_byte = byte & ~(1 << 7);
            const bool shift_too_large_for_result =
                (result.number_of_bytes_read * 7 >= 64) && (masked_byte != ((temp < 0) ? 0x7Fu : 0u));
            const bool shift_too_large_for_byte =
                (result.number_of_bytes_read * 7) == 63 && masked_byte != 0x00 && masked_byte != 0x7Fu;

            if (shift_too_large_for_result || shift_too_large_for_byte)
                return false;

            temp = (temp) | (masked_byte << (result.number_of_bytes_read * 7));
            ++result.number_of_bytes_read;
        } while (byte & (1 << 7));

        if ((result.number_of_bytes_read * 7) < 64 && (byte & 0x40))
        {
            // sign extend
            temp |= ((u64)(-1) << (result.number_of_bytes_read * 7));
        }

        // Now that we've accumulated into an i64, make sure it fits into result
        if constexpr (sizeof(ValueType) < sizeof(u64))
        {
            if (temp > NumericLimits<ValueType>::max() || temp < NumericLimits<ValueType>::min())
                return false;
        }

        result.value = static_cast<ValueType>(temp);

        return true;
    }

    static bool write_leb_signed(OutputStream& stream, i32 value)
    {
        while (true)
        {
            if ((value & 0xFFFFFF80) == 0)
            {
                stream << static_cast<u8>(value);
                return true;
            }

            stream << static_cast<u8>((value & 0x7F) | 0x80);
            value >>= 7;
        }
    }

    static bool write_string(OutputStream& stream, const String& value)
    {
        if (!write_leb_signed(stream, value.length()))
            return false;

        stream.write(value.bytes());
        return true;
    }

    static bool read_string(InputStream& stream, String& value)
    {
        LEBResult<i32> length;
        if (!read_leb_signed(stream, length))
            return false;

        Vector<u8> characters;
        characters.grow_capacity(length.value);
        stream.read({characters.data(), (size_t)length.value});
        value = String(reinterpret_cast<char*>(characters.data()), length.value);
        return true;
    }
};
}