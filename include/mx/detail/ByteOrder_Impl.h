#pragma once

#include <QtEndian>
#include <QList>
#include <QVector>
#include <QString>
#include <tuple>
#include <type_traits>
#include <cstdint>
#include <cstddef>
#include <cstring> // memcpy
#include "mx/detail/QtContainerTraits.h"

namespace mx
{
template<typename T>
void convertNetOrder(T& v);

template<typename T, size_t N>
void convertNetOrder(T (&arr)[N]);

template<typename T>
void convertHostOrder(T& v);

template<typename T, size_t N>
void convertHostOrder(T (&arr)[N]);

template<typename T>
T toNetOrderStruct(const T& s);

template<typename T>
T toHostOrderStruct(const T& s);

template<typename T, typename = void>
struct has_asTuple : std::false_type {};

template<typename T>
struct has_asTuple<T, std::void_t<decltype(std::declval<T&>().asTuple())>> : std::true_type {};

template<typename T, typename = void>
struct has_mx_bitfields_convert : std::false_type {};

template<typename T>
struct has_mx_bitfields_convert<
    T,
    std::void_t<
        decltype(std::declval<T&>().mx_to_net_bitfields()),
        decltype(std::declval<T&>().mx_to_host_bitfields())
    >
> : std::true_type {};

// ====================================================
// 单值字节序转换（整数 + 浮点数）
// ====================================================
template<typename T>
T toNetOrderValue(T v) {
    if constexpr (std::is_enum_v<T>) {
        using U = std::underlying_type_t<T>;
        static_assert(sizeof(U) == 1 || sizeof(U) == 2 || sizeof(U) == 4 || sizeof(U) == 8);
        U u;
        std::memcpy(&u, &v, sizeof(U));

        if constexpr (sizeof(U) == 2) u = qToBigEndian(static_cast<quint16>(u));
        else if constexpr (sizeof(U) == 4) u = qToBigEndian(static_cast<quint32>(u));
        else if constexpr (sizeof(U) == 8) u = qToBigEndian(static_cast<quint64>(u));

        std::memcpy(&v, &u, sizeof(U));
        return v;
    }
    else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) {
        quint32 temp;
        memcpy(&temp, &v, sizeof(v));
        temp = qToBigEndian(temp);
        memcpy(&v, &temp, sizeof(v));
        return v;
    }
    else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) {
        quint64 temp;
        memcpy(&temp, &v, sizeof(v));
        temp = qToBigEndian(temp);
        memcpy(&v, &temp, sizeof(v));
        return v;
    }
    else if constexpr (sizeof(T) == 2)
        return qToBigEndian(v);
    else if constexpr (sizeof(T) == 4)
        return qToBigEndian(v);
    else if constexpr (sizeof(T) == 8)
    {
        //return qToBigEndian(v);
        quint64 temp;
        memcpy(&temp, &v, sizeof(v));
        temp = qToBigEndian(temp);
        memcpy(&v, &temp, sizeof(v));
        return v;
    }
    else
        return v; // 1字节类型无需转换
}

template<typename T>
T toHostOrderValue(T v) {
    if constexpr (std::is_enum_v<T>) {
        using U = std::underlying_type_t<T>;
        static_assert(sizeof(U) == 1 || sizeof(U) == 2 || sizeof(U) == 4 || sizeof(U) == 8);
        U u;
        std::memcpy(&u, &v, sizeof(U));

        if constexpr (sizeof(U) == 2) u = qFromBigEndian(static_cast<quint16>(u));
        else if constexpr (sizeof(U) == 4) u = qFromBigEndian(static_cast<quint32>(u));
        else if constexpr (sizeof(U) == 8) u = qFromBigEndian(static_cast<quint64>(u));

        std::memcpy(&v, &u, sizeof(U));
        return v;
    }
    else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 4) {
        quint32 temp;
        memcpy(&temp, &v, sizeof(v));
        temp = qFromBigEndian(temp);
        memcpy(&v, &temp, sizeof(v));
        return v;
    }
    else if constexpr (std::is_floating_point_v<T> && sizeof(T) == 8) {
        quint64 temp;
        memcpy(&temp, &v, sizeof(v));
        temp = qFromBigEndian(temp);
        memcpy(&v, &temp, sizeof(v));
        return v;
    }
    else if constexpr (sizeof(T) == 2)
        return qFromBigEndian(v);
    else if constexpr (sizeof(T) == 4)
        return qFromBigEndian(v);
    else if constexpr (sizeof(T) == 8)
    {
        //return qFromBigEndian(v);
        quint64 temp;
        memcpy(&temp, &v, sizeof(v));
        temp = qFromBigEndian(temp);
        memcpy(&v, &temp, sizeof(v));
        return v;
    }
    else
        return v;
}

// ====================================================
// 数组字节序转换
// ====================================================
template<typename T, size_t N>
void toNetOrderArray(T (&arr)[N]) {
    for (size_t i = 0; i < N; ++i)
        convertNetOrder(arr[i]);
}

template<typename T, size_t N>
void toHostOrderArray(T (&arr)[N]) {
    for (size_t i = 0; i < N; ++i)
        convertHostOrder(arr[i]);
}

// ====================================================
// 泛型字段处理（值 + 数组）
// ====================================================
template<typename T>
void convertNetOrder(T& v) {
    if constexpr (has_asTuple<T>::value) {
        v = toNetOrderStruct(v);
        if constexpr (has_mx_bitfields_convert<T>::value) {
            v.mx_to_net_bitfields();
        }
    }
    else if constexpr (has_mx_bitfields_convert<T>::value) {
        v.mx_to_net_bitfields();
    }
    else if constexpr (is_q_list<T>::value || is_q_vector<T>::value) {
        for (auto& elem : v) {
            convertNetOrder(elem);
        }
    }
    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        v = toNetOrderValue(v);
    }
}

template<typename T, size_t N>
void convertNetOrder(T (&arr)[N]) {
    toNetOrderArray(arr);
}

template<typename T>
void convertHostOrder(T& v) {
    if constexpr (has_asTuple<T>::value) {
        v = toHostOrderStruct(v);
        if constexpr (has_mx_bitfields_convert<T>::value) {
            v.mx_to_host_bitfields();
        }
    }
    else if constexpr (has_mx_bitfields_convert<T>::value) {
        v.mx_to_host_bitfields();
    }
    else if constexpr (is_q_list<T>::value || is_q_vector<T>::value) {
        for (auto& elem : v) {
            convertHostOrder(elem);
        }
    }
    else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>) {
        v = toHostOrderValue(v);
    }
}

template<typename T, size_t N>
void convertHostOrder(T (&arr)[N]) {
    toHostOrderArray(arr);
}

// ====================================================
// 结构体元组遍历
// ====================================================
template<typename T>
T toNetOrderStruct(const T& s) {
    T result = s;
    auto fields = result.asTuple();
    std::apply([](auto&... field) {
        ((convertNetOrder(field)), ...);
    }, fields);
    return result;
}

template<typename T>
T toHostOrderStruct(const T& s) {
    T result = s;
    auto fields = result.asTuple();
    std::apply([](auto&... field) {
        ((convertHostOrder(field)), ...);
    }, fields);
    return result;
}

// ====================================================
// 对外接口实现
// ====================================================
template<typename T>
T toNetOrder(const T& s) { return toNetOrderStruct(s); }

template<typename T>
T toHostOrder(const T& s) { return toHostOrderStruct(s); }
}

