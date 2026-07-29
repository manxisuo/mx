#pragma once

// 说明：此文件包含 serialize/deserialize 的所有模板实现与特化。
// 设计要点：
//  - 对 POD（trivially copyable）类型使用 memcpy 直接读写。
//  - QString：默认以 uint32_t 写长度，可使用 serializeWithLength/deserializeWithLength 指定其他整数类型。
//  - QList<T> 和 QVector<T>：默认以 uint32_t 写元素个数；也提供 serializeWithLength/deserializeWithLength 支持自定义长度类型。
//  - 自定义类型（非 POD、非 QString、非 QList、非 QVector）需要在结构体内提供 serializeFields / deserializeFields。
//  - 长度和计数的写入/读取目前以宿主字节序直接写入（若需要固定协议字节序请改为网络字节序）。

#include <cstring>
#include <stdexcept>
#include <type_traits>
#include <cstdint>
#include <limits>
#include <string>
#include <optional>
#include <QByteArray>
#include <QString>
#include <QList>
#include <QVector>
#include "mx/detail/QtContainerTraits.h"

namespace mx {

// ---------- 通用长度读写（LenT 必须为整型） ----------
// writeLength 与 readLength 以原始 bytes 写入/读取 LenT 大小的整数（宿主字节序）。
// 如果需要协议上固定字节序（例如大端），在这里做字节序转换（htonl/ntohs 等）。
template<typename LenT>
inline void writeLength(QByteArray &ba, LenT v) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    ba.append(reinterpret_cast<const char*>(&v), static_cast<int>(sizeof(LenT)));
}

template<typename LenT>
inline void readLength(const QByteArray &ba, int &offset, LenT &out) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    int remaining = ba.size() - offset;
    if (remaining < 0 || static_cast<int>(sizeof(LenT)) > remaining) {
        throw std::runtime_error("Buffer underflow (readLength)");
    }
    std::memcpy(&out, ba.constData() + offset, sizeof(LenT));
    offset += static_cast<int>(sizeof(LenT));
}

// ---------- QString（默认使用 uint32_t 存长度） ----------
// serialize 将字符串转成 UTF-8 字节并先写入 uint32_t 长度，再写字节内容。
// deserialize 首先读取 uint32_t 长度，做越界检查，然后构造 QString。
inline void serialize(QByteArray &ba, const QString &value) {
    QByteArray utf8 = value.toUtf8();
    uint32_t byteLen = static_cast<uint32_t>(utf8.size());
    writeLength<uint32_t>(ba, byteLen);
    if (byteLen > 0) ba.append(utf8.constData(), static_cast<int>(byteLen));
}

inline void deserialize(const QByteArray &ba, int &offset, QString &value) {
    uint32_t byteLen = 0;
    readLength<uint32_t>(ba, offset, byteLen);
    int remaining = ba.size() - offset;
    if (static_cast<uint64_t>(byteLen) > static_cast<uint64_t>(remaining)) throw std::runtime_error("QString 读取时缓冲区不足");
    if (byteLen > 0) {
        value = QString::fromUtf8(ba.constData() + offset, static_cast<int>(byteLen));
        offset += static_cast<int>(byteLen);
    } else {
        value.clear();
    }
}

// ---------- QString（显式长度类型版本） ----------
// serializeWithLength/deserializeWithLength 可以指定 LenT（例如 uint32_t、int16_t）
// 会检查溢出（当 utf8 长度超出 LenT 可表达范围时抛错）
template<typename LenT>
void serializeWithLength(QByteArray &ba, const QString &value) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    QByteArray utf8 = value.toUtf8();
    uint64_t needed = static_cast<uint64_t>(utf8.size());
    if (needed > static_cast<uint64_t>(std::numeric_limits<LenT>::max())) {
        throw std::runtime_error("QString length exceeds LenT range");
    }
    LenT byteLen = static_cast<LenT>(utf8.size());
    writeLength<LenT>(ba, byteLen);
    if (byteLen > 0) ba.append(utf8.constData(), static_cast<int>(byteLen));
}

template<typename LenT>
void deserializeWithLength(const QByteArray &ba, int &offset, QString &value) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    LenT byteLen{};
    readLength<LenT>(ba, offset, byteLen);
    if constexpr (std::is_signed_v<LenT>) {
        if (byteLen < 0) throw std::runtime_error("QString length is negative");
    }
    uint64_t ulen = static_cast<uint64_t>(byteLen);
    int remaining = ba.size() - offset;
    if (ulen > static_cast<uint64_t>(remaining)) throw std::runtime_error("deserializeWithLength QString: buffer underflow");
    if (ulen > 0) {
        value = QString::fromUtf8(ba.constData() + offset, static_cast<int>(ulen));
        offset += static_cast<int>(ulen);
    } else {
        value.clear();
    }
}

// ---------- QString（固定大小版本，无长度前缀） ----------
// serializeFixedSize/deserializeFixedSize 用于固定大小的字符串字段（类似 char name[64]）
// 将 QString 转为 UTF-8 后填充到固定字节数，不足部分用 0 填充，超出部分截断
template<int N>
void serializeFixedSize(QByteArray &ba, const QString &value) {
    QByteArray utf8 = value.toUtf8();
    int copyLen = std::min(utf8.size(), N);
    if (copyLen > 0) {
        ba.append(utf8.constData(), copyLen);
    }
    // 填充剩余字节为 0
    int remaining = N - copyLen;
    if (remaining > 0) {
        ba.append(remaining, '\0');
    }
}

template<int N>
void deserializeFixedSize(const QByteArray &ba, int &offset, QString &value) {
    int remaining = ba.size() - offset;
    if (remaining < N) {
        throw std::runtime_error("deserializeFixedSize QString: buffer underflow");
    }
    // 查找字符串结束位置（遇到 '\0' 或到达固定大小）
    const char* data = ba.constData() + offset;
    int len = 0;
    while (len < N && data[len] != '\0') {
        ++len;
    }
    if (len > 0) {
        value = QString::fromUtf8(data, len);
    } else {
        value.clear();
    }
    offset += N;
}

// ---------- 辅助函数：处理整数参数（固定大小字符串） ----------
// 用于 MX_FIELD(name, 整数) 宏，处理固定大小字符串序列化
// 使用 std::integral_constant 将整数字面量转换为编译时常量
template<typename T, int N>
auto _mx_field_helper_impl(QByteArray &ba, const T &value, std::integral_constant<int, N>) ->
    std::enable_if_t<std::is_same_v<T, QString>> {
    serializeFixedSize<N>(ba, value);
}

// 非 QString 类型的默认处理
template<typename T, typename Arg>
auto _mx_field_helper_impl(QByteArray &ba, const T &value, Arg) ->
    std::enable_if_t<!std::is_same_v<T, QString>> {
    serialize(ba, value);
}

// 反序列化辅助函数
template<typename T, int N>
auto _mx_field_from_helper_impl(const QByteArray &ba, int &offset, T &value, std::integral_constant<int, N>) ->
    std::enable_if_t<std::is_same_v<T, QString>> {
    deserializeFixedSize<N>(ba, offset, value);
}

// 重载：接受整数字面量（运行时值，不应该被调用）
template<typename T>
auto _mx_field_from_helper_impl(const QByteArray &ba, int &offset, T &value, int size) ->
    std::enable_if_t<std::is_same_v<T, QString>> {
    static_assert(sizeof(T) == 0, "MX_FIELD_FROM(name, size) requires a compile-time integer literal");
}

template<typename T>
auto _mx_field_from_helper_impl(const QByteArray &ba, int &offset, T &value, long size) ->
    std::enable_if_t<std::is_same_v<T, QString>> {
    static_assert(sizeof(T) == 0, "MX_FIELD_FROM(name, size) requires a compile-time integer literal");
}

template<typename T>
auto _mx_field_from_helper_impl(const QByteArray &ba, int &offset, T &value, long long size) ->
    std::enable_if_t<std::is_same_v<T, QString>> {
    static_assert(sizeof(T) == 0, "MX_FIELD_FROM(name, size) requires a compile-time integer literal");
}

template<typename T, typename LenT>
auto _mx_field_from_helper_impl(const QByteArray &ba, int &offset, T &value, LenT) ->
    std::enable_if_t<std::is_integral_v<decltype(LenT{})> && std::is_same_v<T, QString>> {
    deserializeWithLength<decltype(LenT{})>(ba, offset, value);
}

template<typename T, typename Arg>
auto _mx_field_from_helper_impl(const QByteArray &ba, int &offset, T &value, Arg) ->
    std::enable_if_t<!std::is_same_v<T, QString>> {
    deserialize(ba, offset, value);
}

// ---------- 优先：带 serializeFields / deserializeFields 的自定义类型 ----------
// 即使类型是 trivially copyable（例如仅含位域的结构体），也走显式字段协议，
// 避免依赖编译器内存布局，保证跨平台一致。
template<typename T>
std::enable_if_t<
    has_serializeFields<T>::value &&
        !std::is_same_v<T, QString> &&
        !is_q_list<T>::value &&
        !is_q_vector<T>::value,
    void>
serialize(QByteArray &ba, const T &value) {
    value.serializeFields(ba);
}

template<typename T>
std::enable_if_t<
    has_deserializeFields<T>::value &&
        !std::is_same_v<T, QString> &&
        !is_q_list<T>::value &&
        !is_q_vector<T>::value,
    void>
deserialize(const QByteArray &ba, int &offset, T &value) {
    value.deserializeFields(ba, offset);
}

// ---------- POD（trivially copyable，且未提供 serializeFields） ----------
// 对于可平凡复制的类型直接 memcpy 到字节数组中（注意字节序问题：当前为宿主字节序）
template<typename T>
std::enable_if_t<
    !has_serializeFields<T>::value &&
        std::is_trivially_copyable_v<T> &&
        !std::is_same_v<T, QString> &&
        !is_q_list<T>::value &&
        !is_q_vector<T>::value,
    void>
serialize(QByteArray &ba, const T &value) {
    ba.append(reinterpret_cast<const char*>(&value), static_cast<int>(sizeof(T)));
}

template<typename T>
std::enable_if_t<
    !has_deserializeFields<T>::value &&
        std::is_trivially_copyable_v<T> &&
        !std::is_same_v<T, QString> &&
        !is_q_list<T>::value &&
        !is_q_vector<T>::value,
    void>
deserialize(const QByteArray &ba, int &offset, T &value) {
    int remaining = ba.size() - offset;
    if (remaining < 0 || static_cast<int>(sizeof(T)) > remaining) {
        throw std::runtime_error("POD deserialize: buffer underflow");
    }
    std::memcpy(&value, ba.constData() + offset, sizeof(T));
    offset += static_cast<int>(sizeof(T));
}

// ---------- QList<T> 默认实现（使用 uint32_t 记录长度） ----------
// 写入：先写 uint32_t count，再对每个元素递归 serialize。
// 读出：先读 uint32_t count，然后逐个构造元素并反序列化。
template<typename T>
void serialize(QByteArray &ba, const QList<T> &list) {
    uint32_t count = static_cast<uint32_t>(list.size());
    writeLength<uint32_t>(ba, count);
    for (const auto &elem : list) {
        serialize(ba, elem);
    }
}

template<typename T>
void deserialize(const QByteArray &ba, int &offset, QList<T> &list) {
    uint32_t count = 0;
    readLength<uint32_t>(ba, offset, count);
    list.clear();
    list.reserve(static_cast<int>(count));
    for (uint32_t i = 0; i < count; ++i) {
        T elem{};
        deserialize(ba, offset, elem);
        list.append(elem);
    }
}

// ---------- QList<T> 显式长度类型版本（模板） ----------
// serializeWithLength<LenT> 允许用不同大小的整数表示元素个数（例如 int16_t、uint32_t 等）
// deserializeWithLength<LenT> 同理，并做安全性检查（负值、溢出等）
template<typename LenT, typename T>
void serializeWithLength(QByteArray &ba, const QList<T> &list) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    uint64_t needed = static_cast<uint64_t>(list.size());
    if (needed > static_cast<uint64_t>(std::numeric_limits<LenT>::max())) {
        throw std::runtime_error("QList size exceeds LenT range");
    }
    LenT count = static_cast<LenT>(list.size());
    writeLength<LenT>(ba, count);
    for (const auto &elem : list) serialize(ba, elem);
}

template<typename LenT, typename T>
void deserializeWithLength(const QByteArray &ba, int &offset, QList<T> &list) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    LenT count{};
    readLength<LenT>(ba, offset, count);
    if constexpr (std::is_signed_v<LenT>) {
        if (count < 0) throw std::runtime_error("QList size is negative");
    }
    int64_t ucount = static_cast<int64_t>(count);
    if (ucount < 0) throw std::runtime_error("QList size is invalid");
    list.clear();
    list.reserve(static_cast<int>(ucount));
    for (int64_t i = 0; i < ucount; ++i) {
        T elem{};
        deserialize(ba, offset, elem);
        list.append(elem);
    }
}

// ---------- QVector<T> 默认实现（使用 uint32_t 记录长度） ----------
// 写入：先写 uint32_t count，再对每个元素递归 serialize。
// 读出：先读 uint32_t count，然后逐个构造元素并反序列化。
template<typename T>
void serialize(QByteArray &ba, const QVector<T> &vec) {
    uint32_t count = static_cast<uint32_t>(vec.size());
    writeLength<uint32_t>(ba, count);
    for (const auto &elem : vec) {
        serialize(ba, elem);
    }
}

template<typename T>
void deserialize(const QByteArray &ba, int &offset, QVector<T> &vec) {
    uint32_t count = 0;
    readLength<uint32_t>(ba, offset, count);
    vec.clear();
    vec.reserve(static_cast<int>(count));
    for (uint32_t i = 0; i < count; ++i) {
        T elem{};
        deserialize(ba, offset, elem);
        vec.append(elem);
    }
}

// ---------- QVector<T> 显式长度类型版本（模板） ----------
// serializeWithLength<LenT> 允许用不同大小的整数表示元素个数（例如 int16_t、uint32_t 等）
// deserializeWithLength<LenT> 同理，并做安全性检查（负值、溢出等）
template<typename LenT, typename T>
void serializeWithLength(QByteArray &ba, const QVector<T> &vec) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    uint64_t needed = static_cast<uint64_t>(vec.size());
    if (needed > static_cast<uint64_t>(std::numeric_limits<LenT>::max())) {
        throw std::runtime_error("QVector size exceeds LenT range");
    }
    LenT count = static_cast<LenT>(vec.size());
    writeLength<LenT>(ba, count);
    for (const auto &elem : vec) serialize(ba, elem);
}

template<typename LenT, typename T>
void deserializeWithLength(const QByteArray &ba, int &offset, QVector<T> &vec) {
    static_assert(std::is_integral_v<LenT>, "LenT must be an integral type");
    LenT count{};
    readLength<LenT>(ba, offset, count);
    if constexpr (std::is_signed_v<LenT>) {
        if (count < 0) throw std::runtime_error("QVector size is negative");
    }
    int64_t ucount = static_cast<int64_t>(count);
    if (ucount < 0) throw std::runtime_error("QVector size is invalid");
    vec.clear();
    vec.reserve(static_cast<int>(ucount));
    for (int64_t i = 0; i < ucount; ++i) {
        T elem{};
        deserialize(ba, offset, elem);
        vec.append(elem);
    }
}

// ---------- toByteArray / fromByteArray ----------
// toByteArray: 创建 QByteArray 并调用对象的 serializeFields 将字段写入
// fromByteArray: 构造对象并调用 deserializeFields 从字节流中读取字段
// fromByteArraySafe: 安全版本，返回 std::optional，失败时返回 std::nullopt

template<typename T>
std::enable_if_t<has_serializeFields<T>::value, QByteArray>
toByteArray(const T &obj) {
    QByteArray ba;
    obj.serializeFields(ba);
    return ba;
}

template<typename T>
std::enable_if_t<has_deserializeFields<T>::value, T>
fromByteArray(const QByteArray &ba) {
    T obj;
    int offset = 0;
    obj.deserializeFields(ba, offset);
    // 验证完整性：反序列化后 offset 应该等于 ba.size()
    if (offset != ba.size()) {
        throw std::runtime_error("反序列化数据不完整：读取了 " + std::to_string(offset) + 
                                 " 字节，但缓冲区有 " + std::to_string(ba.size()) + " 字节");
    }
    return obj;
}

template<typename T>
std::enable_if_t<has_deserializeFields<T>::value, std::optional<T>>
fromByteArraySafe(const QByteArray &ba) {
    try {
        T obj;
        int offset = 0;
        obj.deserializeFields(ba, offset);
        // 验证完整性：反序列化后 offset 应该等于 ba.size()
        if (offset != ba.size()) {
            return std::nullopt;
        }
        return obj;
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace mx

