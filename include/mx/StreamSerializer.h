#pragma once

#include <QByteArray>
#include <QDataStream>
#include <optional>
#include <type_traits>
#include "mx/detail/StreamSerializerDefine.h"

namespace mx
{
    // 默认使用 Qt 最新的 QDataStream 版本
    inline constexpr QDataStream::Version StreamVersion = QDataStream::Qt_5_12;

    // 枚举
    template<typename E>
    std::enable_if_t<std::is_enum_v<E>, QDataStream&>
    operator<<(QDataStream& out, const E& e)
    {
        using U = std::underlying_type_t<E>;
        return out << static_cast<U>(e);
    }

    // 枚举
    template<typename E>
    std::enable_if_t<std::is_enum_v<E>, QDataStream&>
    operator>>(QDataStream& in, E& e)
    {
        using U = std::underlying_type_t<E>;
        U v{};
        in >> v;
        e = static_cast<E>(v);
        return in;
    }

    // 将对象序列化为字节数组
    template<typename T>
    QByteArray streamSerialize(const T& object, QDataStream::ByteOrder order = QDataStream::BigEndian)
    {
        QByteArray buffer;
        QDataStream stream(&buffer, QIODevice::WriteOnly);
        stream.setByteOrder(order);
        stream.setVersion(StreamVersion);
        stream << object;
        return buffer;
    }

    // 从字节数组反序列化对象
    template<typename T>
    T streamDeserialize(const QByteArray& buffer, QDataStream::ByteOrder order = QDataStream::BigEndian)
    {
        T object;
        QDataStream stream(buffer);
        stream.setByteOrder(order);
        stream.setVersion(StreamVersion);
        stream >> object;

        return object;
    }

    // 从字节数组反序列化对象（安全版）
    template<typename T>
    std::optional<T> streamDeserializeSafe(const QByteArray& buffer, QDataStream::ByteOrder order = QDataStream::BigEndian)
    {
        T object;
        QDataStream stream(buffer);
        stream.setByteOrder(order);
        stream.setVersion(StreamVersion);
        stream >> object;

        if (stream.status() != QDataStream::Ok)
        {
            return std::nullopt;
        }

        return object;
    }
}


