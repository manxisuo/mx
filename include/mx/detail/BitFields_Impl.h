#pragma once

#include <cstdint>
#include <type_traits>

namespace mx {
namespace detail {

// 按参数顺序将各位打包到整数：第 0 个字段 → bit0，第 1 个 → bit1，……
// 通过按值读取位域，避免对位域取引用。
template<typename... Bits>
inline uint8_t packBitsU8(Bits... bits)
{
    static_assert(sizeof...(Bits) <= 8, "MX_BITFIELDS_U8: at most 8 bit fields");
    uint8_t v = 0;
    int i = 0;
    ((v = static_cast<uint8_t>(v | (static_cast<unsigned>(bits)
                                        ? static_cast<uint8_t>(1u << i)
                                        : uint8_t{0})),
      ++i),
     ...);
    return v;
}

template<typename... Bits>
inline uint16_t packBitsU16(Bits... bits)
{
    static_assert(sizeof...(Bits) <= 16, "MX_BITFIELDS_U16: at most 16 bit fields");
    uint16_t v = 0;
    int i = 0;
    ((v = static_cast<uint16_t>(v | (static_cast<unsigned>(bits)
                                         ? static_cast<uint16_t>(1u << i)
                                         : uint16_t{0})),
      ++i),
     ...);
    return v;
}

} // namespace detail
} // namespace mx

// 位域不能绑定到非 const 引用，unpack 必须在宏里直接赋值。
#define MX_BF_ASSIGN(packed, idx, field) \
    (field) = ((((packed) >> (idx)) & 1))

#define MX_BF_COUNT_IMPL( \
    _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, N, ...) N
#define MX_BF_COUNT(...) \
    MX_BF_COUNT_IMPL(__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define MX_BF_UNPACK_1(p, a1) \
    MX_BF_ASSIGN(p, 0, a1);
#define MX_BF_UNPACK_2(p, a1, a2) \
    MX_BF_UNPACK_1(p, a1) MX_BF_ASSIGN(p, 1, a2);
#define MX_BF_UNPACK_3(p, a1, a2, a3) \
    MX_BF_UNPACK_2(p, a1, a2) MX_BF_ASSIGN(p, 2, a3);
#define MX_BF_UNPACK_4(p, a1, a2, a3, a4) \
    MX_BF_UNPACK_3(p, a1, a2, a3) MX_BF_ASSIGN(p, 3, a4);
#define MX_BF_UNPACK_5(p, a1, a2, a3, a4, a5) \
    MX_BF_UNPACK_4(p, a1, a2, a3, a4) MX_BF_ASSIGN(p, 4, a5);
#define MX_BF_UNPACK_6(p, a1, a2, a3, a4, a5, a6) \
    MX_BF_UNPACK_5(p, a1, a2, a3, a4, a5) MX_BF_ASSIGN(p, 5, a6);
#define MX_BF_UNPACK_7(p, a1, a2, a3, a4, a5, a6, a7) \
    MX_BF_UNPACK_6(p, a1, a2, a3, a4, a5, a6) MX_BF_ASSIGN(p, 6, a7);
#define MX_BF_UNPACK_8(p, a1, a2, a3, a4, a5, a6, a7, a8) \
    MX_BF_UNPACK_7(p, a1, a2, a3, a4, a5, a6, a7) MX_BF_ASSIGN(p, 7, a8);
#define MX_BF_UNPACK_9(p, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
    MX_BF_UNPACK_8(p, a1, a2, a3, a4, a5, a6, a7, a8) MX_BF_ASSIGN(p, 8, a9);
#define MX_BF_UNPACK_10(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
    MX_BF_UNPACK_9(p, a1, a2, a3, a4, a5, a6, a7, a8, a9) MX_BF_ASSIGN(p, 9, a10);
#define MX_BF_UNPACK_11(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
    MX_BF_UNPACK_10(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) MX_BF_ASSIGN(p, 10, a11);
#define MX_BF_UNPACK_12(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) \
    MX_BF_UNPACK_11(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) MX_BF_ASSIGN(p, 11, a12);
#define MX_BF_UNPACK_13(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
    MX_BF_UNPACK_12(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) MX_BF_ASSIGN(p, 12, a13);
#define MX_BF_UNPACK_14(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) \
    MX_BF_UNPACK_13(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) MX_BF_ASSIGN(p, 13, a14);
#define MX_BF_UNPACK_15(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
    MX_BF_UNPACK_14(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) MX_BF_ASSIGN(p, 14, a15);
#define MX_BF_UNPACK_16(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) \
    MX_BF_UNPACK_15(p, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) MX_BF_ASSIGN(p, 15, a16);

#define MX_BF_CONCAT_INNER(a, b) a##b
#define MX_BF_CONCAT(a, b) MX_BF_CONCAT_INNER(a, b)

// 先展开 MX_BF_COUNT，再拼出 MX_BF_UNPACK_N（兼容 MSVC 预处理器）
#define MX_BF_UNPACK(packed, ...) MX_BF_UNPACK_I(MX_BF_COUNT(__VA_ARGS__), packed, __VA_ARGS__)
#define MX_BF_UNPACK_I(n, packed, ...) MX_BF_UNPACK_II(n, packed, __VA_ARGS__)
#define MX_BF_UNPACK_II(n, packed, ...) MX_BF_UNPACK_##n(packed, __VA_ARGS__)
