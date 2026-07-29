#pragma once

#include <tuple>
#include <cstdint>
#include <cstddef>

// 宏：在结构体中声明字段顺序
#define MX_BYTEODER(TYPE, ...) \
auto asTuple() { \
        return std::tie(__VA_ARGS__); \
} \
auto asTuple() const { \
    return std::tie(__VA_ARGS__); \
}

namespace mx
{
    // 将结构体从主机字节序转换为网络字节序
    template<typename T>
    T toNetOrder(const T& s);

    // 将结构体从网络字节序转换为主机字节序
    template<typename T>
    T toHostOrder(const T& s);
}

// 引入内部模板实现
#include "mx/detail/ByteOrder_Impl.h"
