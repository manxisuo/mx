#pragma once

#include <QDataStream>
#include <optional>

namespace MX::StreamSerializer
{
    // 写入所有字段（C++17 fold expression）
    template<typename T, typename... M>
    QDataStream& writeAll(QDataStream& out, const T& obj, M T::*... members)
    {
        ((out << obj.*members), ...);
        return out;
    }

    // 读取所有字段
    template<typename T, typename... M>
    QDataStream& readAll(QDataStream& in, T& obj, M T::*... members)
    {
        ((in >> obj.*members), ...);
        return in;
    }
}

// 序列化宏，前缀化为 MX_SERIALIZABLE
#define MX_SERIALIZABLE(Type, ...)                                \
inline QDataStream& operator<<(QDataStream& out, const Type& obj) \
{                                                                 \
    return MX::StreamSerializer::writeAll(out, obj, __VA_ARGS__);    \
}                                                                 \
inline QDataStream& operator>>(QDataStream& in, Type& obj)        \
{                                                                 \
    return MX::StreamSerializer::readAll(in, obj, __VA_ARGS__);      \
}
