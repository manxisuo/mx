#pragma once

// FieldSerializer.h（C++17 版本）
// 说明：
//  - MX_FIELDS(...) 使用 C++17 的 std::apply + 折叠表达式自动生成 serializeFields 与 deserializeFields，
//    对字段数量没有人工上限（受编译器能力限制，通常很大）。
//  - MX_FIELDS(...) 只接受字段名列表（例如：MX_FIELDS(id, name, bookIdList)）。
//    若某字段需要自定义长度类型（例如 name 的长度用 uint32_t），请使用 MX_FIELD(name, uint32_t) 和 MX_FIELD_FROM(name, uint32_t)
//    或不将该字段放入 MX_FIELDS 中，手动编写序列化/反序列化逻辑。
//  - MX_BITFIELDS_U8 / MX_BITFIELDS_U16：将 C++ 位域按参数顺序显式打包为整数后再序列化，
//    不依赖编译器位域内存布局，可用于跨平台协议。
//
// 用法示例：
// struct Course {
//     uint32_t sn;
//     QString name;
//     MX_FIELDS(sn, name)
// };
//
// 如果需要单独为某字段指定长度类型或固定大小：
// struct Foo {
//     QString name1; // 需要 uint32_t 记录长度
//     QString name2; // 需要固定 64 字节
//     MX_SERIALIZE {
//         MX_FIELD_TYPE(name1, uint32_t);  // 使用 MX_FIELD_TYPE 指定长度类型
//         MX_FIELD(name2, 64);             // 使用整数参数指定固定大小（64 字节）
//     }
//     MX_DESERIALIZE {
//         MX_FIELD_FROM_TYPE(name1, uint32_t);
//         MX_FIELD_FROM(name2, 64);
//     }
// };

#include <QByteArray>
#include <QString>
#include <QList>
#include <QVector>
#include <QtEndian>
#include <type_traits>
#include <cstdint>
#include <tuple>
#include <utility>
#include <optional>

namespace mx {
// 对外接口（实现位于 FieldSerializer_impl.h）

// 检查类型是否有 serializeFields 方法（用于 SFINAE）
template<typename T>
auto has_serializeFields_impl(int) -> decltype(
    std::declval<const T&>().serializeFields(std::declval<QByteArray&>()),
    std::true_type{}
);
template<typename T>
std::false_type has_serializeFields_impl(...);

template<typename T>
struct has_serializeFields : decltype(has_serializeFields_impl<T>(0)) {};

// 检查类型是否有 deserializeFields 方法（用于 SFINAE）
template<typename T>
auto has_deserializeFields_impl(int) -> decltype(
    std::declval<T&>().deserializeFields(std::declval<const QByteArray&>(), std::declval<int&>()),
    std::true_type{}
);
template<typename T>
std::false_type has_deserializeFields_impl(...);

template<typename T>
struct has_deserializeFields : decltype(has_deserializeFields_impl<T>(0)) {};

// 序列化：将对象转换为字节数组
// 要求：类型 T 必须有 serializeFields(QByteArray&) const 方法
template<typename T>
std::enable_if_t<has_serializeFields<T>::value, QByteArray>
toByteArray(const T &obj);

// 反序列化：从字节数组构造对象（抛出异常版本）
// 要求：类型 T 必须有 deserializeFields(const QByteArray&, int&) 方法
// 如果反序列化失败或数据不完整，会抛出 std::runtime_error
template<typename T>
std::enable_if_t<has_deserializeFields<T>::value, T>
fromByteArray(const QByteArray &ba);

// 反序列化：从字节数组构造对象（安全版本，返回 std::optional）
// 如果反序列化失败或数据不完整，返回 std::nullopt
template<typename T>
std::enable_if_t<has_deserializeFields<T>::value, std::optional<T>>
fromByteArraySafe(const QByteArray &ba);

// 注意具体的 serialize/deserialize / serializeWithLength / deserializeWithLength
// 的实现应该放在 FieldSerializer_impl.h 中（与本头文件配套使用）。
} // namespace mx

// --------------------------- 宏定义 ---------------------------

// 可变参数选择辅助（1 or 2 args）
#define MX_PP_GET_MACRO(_1,_2,NAME,...) NAME

// 单字段序列化宏（支持两种形式）：
//   - MX_FIELD(name)                    : 使用默认序列化
//   - MX_FIELD(name, 整数)              : 固定大小字符串（如 10 表示 10 字节），仅对 QString 有效
// 注意：整数参数仅对 QString 类型有效，其他类型会使用默认序列化
// 注意：指定长度类型请使用 MX_FIELD_TYPE(name, LenType)，而不是 MX_FIELD(name, LenType)
#define MX_FIELD_1(NAME)            mx::serialize(ba, NAME)
#define MX_FIELD_2(NAME, ARG)       mx::_mx_field_helper_impl(ba, NAME, std::integral_constant<int, ARG>{})
#define MX_FIELD(...)               MX_PP_GET_MACRO(__VA_ARGS__, MX_FIELD_2, MX_FIELD_1)(__VA_ARGS__)

// 类型参数专用宏（用于指定长度类型，更简洁的语法）
#define MX_FIELD_TYPE(NAME, LENTYPE) mx::serializeWithLength<LENTYPE>(ba, NAME)
#define MX_FIELD_FROM_TYPE(NAME, LENTYPE) mx::deserializeWithLength<LENTYPE>(ba, offset, NAME)

// 单字段反序列化宏
#define MX_FIELD_FROM_1(NAME)       mx::deserialize(ba, offset, NAME)
// 对于整数参数：使用 std::integral_constant 将整数字面量转换为编译时常量
// 对于类型参数：直接展开为 deserializeWithLength<Type>（通过宏生成模板语法）
#define MX_FIELD_FROM_2(NAME, ARG)  mx::_mx_field_from_helper_impl(ba, offset, NAME, std::integral_constant<int, ARG>{})
#define MX_FIELD_FROM(...)          MX_PP_GET_MACRO(__VA_ARGS__, MX_FIELD_FROM_2, MX_FIELD_FROM_1)(__VA_ARGS__)

// 方法头宏（无括号，像函数定义）
#define MX_SERIALIZE    void serializeFields(QByteArray &ba) const
#define MX_DESERIALIZE  void deserializeFields(const QByteArray &ba, int &offset)

// --------------------------- MX_FIELDS（C++17 实现） ---------------------------
// 说明：MX_FIELDS(...) 接受任意数量的字段名（字段名之间以逗号分隔），例：MX_FIELDS(a, b, c)
// 它会在类内部生成两个方法：serializeFields(QByteArray&) const 和 deserializeFields(const QByteArray&, int&)
// 注意：如果字段里包含需要显式长度类型的成员（比如想把 QString 的长度用 uint16_t），请不要把该字段放到 MX_FIELDS 中，
//       而是使用 MX_FIELD(name, uint16_t) 和 MX_FIELD_FROM(name, uint16_t) 来覆盖实现。

#define MX_FIELDS(...) \
void serializeFields(QByteArray &ba) const { \
        /* 使用 tie + apply + 折叠表达式对任意数量字段进行序列化（避免悬垂引用） */ \
        auto tpl = std::tie(__VA_ARGS__); \
        std::apply([&](auto&... args){ ( (mx::serialize(ba, args)), ... ); }, tpl); \
} \
    void deserializeFields(const QByteArray &ba, int &offset) { \
        /* 对任意数量字段进行反序列化 */ \
        auto tpl = std::tie(__VA_ARGS__); \
        std::apply([&](auto&... args){ ( (mx::deserialize(ba, offset, args)), ... ); }, tpl); \
}

// --------------------------- MX_BITFIELDS（跨平台安全位域打包） ---------------------------
// 将 C++ 位域按参数顺序显式打包为整数后再序列化，不依赖编译器位域内存布局。
// 约定：第 0 个字段 → bit0，第 1 个 → bit1，以此类推。
// 用法（写在位域结构体内部，不要对各位使用 MX_FIELDS / MX_BYTEODER）：
//   struct AState {
//       unsigned char b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
//       MX_BITFIELDS_U8(b0, b1, b2, b3, b4, b5, b6, b7)
//   };
#include "mx/detail/BitFields_Impl.h"

#define MX_BITFIELDS_U8(...) \
    uint8_t mx_pack_bits() const { \
        return ::mx::detail::packBitsU8(__VA_ARGS__); \
    } \
    void mx_unpack_bits(uint8_t packed) { \
        MX_BF_UNPACK(packed, __VA_ARGS__); \
    } \
    void mx_to_net_bitfields() {} \
    void mx_to_host_bitfields() {} \
    void serializeFields(QByteArray &ba) const { \
        ::mx::serialize(ba, mx_pack_bits()); \
    } \
    void deserializeFields(const QByteArray &ba, int &offset) { \
        uint8_t packed{}; \
        ::mx::deserialize(ba, offset, packed); \
        mx_unpack_bits(packed); \
    }

#define MX_BITFIELDS_U16(...) \
    uint16_t mx_pack_bits() const { \
        return ::mx::detail::packBitsU16(__VA_ARGS__); \
    } \
    void mx_unpack_bits(uint16_t packed) { \
        MX_BF_UNPACK(packed, __VA_ARGS__); \
    } \
    void mx_to_net_bitfields() { \
        uint16_t packed = mx_pack_bits(); \
        packed = qToBigEndian(packed); \
        mx_unpack_bits(packed); \
    } \
    void mx_to_host_bitfields() { \
        uint16_t packed = mx_pack_bits(); \
        packed = qFromBigEndian(packed); \
        mx_unpack_bits(packed); \
    } \
    void serializeFields(QByteArray &ba) const { \
        ::mx::serialize(ba, mx_pack_bits()); \
    } \
    void deserializeFields(const QByteArray &ba, int &offset) { \
        uint16_t packed{}; \
        ::mx::deserialize(ba, offset, packed); \
        mx_unpack_bits(packed); \
    }

// --------------------------- 包含实现 ---------------------------
#include "mx/detail/FieldSerializer_impl.h"

