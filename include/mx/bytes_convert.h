#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace TQ {
namespace detail {
    inline bool isLittleEndian()
    {
        const uint16_t value = 1;
        return *reinterpret_cast<const uint8_t*>(&value) == 1;
    }

    inline uint16_t byteSwap(uint16_t value)
    {
        return static_cast<uint16_t>((value >> 8) | (value << 8));
    }

    inline uint32_t byteSwap(uint32_t value)
    {
        return ((value & 0x000000ffU) << 24) |
               ((value & 0x0000ff00U) << 8) |
               ((value & 0x00ff0000U) >> 8) |
               ((value & 0xff000000U) >> 24);
    }

    inline uint64_t byteSwap(uint64_t value)
    {
        return ((value & 0x00000000000000ffULL) << 56) |
               ((value & 0x000000000000ff00ULL) << 40) |
               ((value & 0x0000000000ff0000ULL) << 24) |
               ((value & 0x00000000ff000000ULL) << 8) |
               ((value & 0x000000ff00000000ULL) >> 8) |
               ((value & 0x0000ff0000000000ULL) >> 24) |
               ((value & 0x00ff000000000000ULL) >> 40) |
               ((value & 0xff00000000000000ULL) >> 56);
    }

    template<typename SignedT, typename UnsignedT>
    inline SignedT convertSigned(SignedT value)
    {
        UnsignedT raw{};
        std::memcpy(&raw, &value, sizeof(value));
        raw = isLittleEndian() ? byteSwap(raw) : raw;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    }
}

    inline uint8_t fromNet(uint8_t a) { return a; }
    inline uint16_t fromNet(uint16_t a) { return detail::isLittleEndian() ? detail::byteSwap(a) : a; }
    inline uint32_t fromNet(uint32_t a) { return detail::isLittleEndian() ? detail::byteSwap(a) : a; }
    inline uint64_t fromNet(uint64_t a) { return detail::isLittleEndian() ? detail::byteSwap(a) : a; }

    inline int8_t fromNet(int8_t a) { return a; }
    inline int16_t fromNet(int16_t a) { return detail::convertSigned<int16_t, uint16_t>(a); }
    inline int32_t fromNet(int32_t a) { return detail::convertSigned<int32_t, uint32_t>(a); }
    inline int64_t fromNet(int64_t a) { return detail::convertSigned<int64_t, uint64_t>(a); }

    inline uint8_t toNet(uint8_t a) { return a; }
    inline uint16_t toNet(uint16_t a) { return fromNet(a); }
    inline uint32_t toNet(uint32_t a) { return fromNet(a); }
    inline uint64_t toNet(uint64_t a) { return fromNet(a); }

    inline int8_t toNet(int8_t a) { return a; }
    inline int16_t toNet(int16_t a) { return fromNet(a); }
    inline int32_t toNet(int32_t a) { return fromNet(a); }
    inline int64_t toNet(int64_t a) { return fromNet(a); }

    inline uint16_t BCD2Dec(uint16_t data)
    {
        return static_cast<uint16_t>(((data & 0xf000) >> 12) * 1000 +
                                     ((data & 0x0f00) >> 8) * 100 +
                                     ((data & 0x00f0) >> 4) * 10 +
                                     (data & 0x000f));
    }

    inline uint64_t BCD2Dec(uint64_t data)
    {
        char str[64];
        std::snprintf(str, sizeof(str), "%llx", static_cast<unsigned long long>(data));
        return static_cast<uint64_t>(std::strtoull(str, nullptr, 10));
    }

    inline uint16_t Dec2BCD(uint16_t data)
    {
        return static_cast<uint16_t>((((data % 10000) / 1000) << 12) +
                                     (((data % 1000) / 100) << 8) +
                                     (((data % 100) / 10) << 4) +
                                     (data % 10));
    }
}
