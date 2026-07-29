# mx

`mx` 是一个仅头文件（header-only）的 C++17 辅助库，面向基于 Qt 的项目。它提供以下小型工具：

- 反射结构体的字节序转换；
- 字段序列化 / 反序列化到 `QByteArray`；
- 基于 `QDataStream` 的序列化辅助；
- 字节转换与 BCD 转换辅助；
- 位域结构体的跨平台安全打包（`MX_BITFIELDS_U8` / `MX_BITFIELDS_U16`）。

库支持常见的 Qt 类型，例如 `QString`、`QList` 和 `QVector`。

## 目录结构

```text
mx/
├─ CMakeLists.txt
├─ README.md
├─ include/
│  └─ mx/
│     ├─ mx.h
│     ├─ ByteOrder.h
│     ├─ FieldSerializer.h
│     ├─ StreamSerializer.h
│     ├─ SignalUtil.h
│     ├─ MXSignals.h
│     ├─ bytes_convert.h
│     └─ detail/
│        ├─ BitFields_Impl.h
│        ├─ ByteOrder_Impl.h
│        ├─ FieldSerializer_impl.h
│        └─ StreamSerializerDefine.h
├─ examples/
└─ tests/
```

公开头文件位于 `include/mx`。`include/mx/detail` 下的文件为实现细节，用户代码不应直接包含它们。

## 依赖要求

- C++17
- Qt 5 Core 模块

当前 CMake 目标使用 `Qt5::Core`。在 MSVC 下还会自动启用 `/utf-8` 与 `/Zc:preprocessor`（后者为位域宏正确展开所必需）。

## CMake 用法

将本仓库作为子目录添加：

```cmake
add_subdirectory(path/to/mx)
target_link_libraries(your_target PRIVATE mx::mx)
```

然后包含头文件：

```cpp
#include <mx/mx.h>
```

或仅包含所需模块：

```cpp
#include <mx/ByteOrder.h>
#include <mx/FieldSerializer.h>
```

## 字节序示例

```cpp
#include <mx/ByteOrder.h>

#include <QList>
#include <QString>
#include <cstdint>

struct Item
{
    uint16_t id{};
    QString name;
    MX_BYTEODER(Item, id, name)
};

struct Packet
{
    uint32_t sequence{};
    QList<Item> items;
    MX_BYTEODER(Packet, sequence, items)
};

Packet net = mx::toNetOrder(packet);
Packet host = mx::toHostOrder(net);
```

`MX_BYTEODER` 通过 `asTuple()` 暴露字段顺序。数值与枚举字段会递归转换。嵌套结构体、C 数组、`QList<T>` 和 `QVector<T>` 在其元素类型同样受支持时均可使用。像 `QString` 这类与字节序无关的字段会保持不变。

## 字段序列化示例

```cpp
#include <mx/FieldSerializer.h>

#include <QString>
#include <QList>
#include <cstdint>

struct Course
{
    uint32_t id{};
    QString name;
    QList<uint32_t> studentIds;

    MX_FIELDS(id, name, studentIds)
};

QByteArray bytes = mx::toByteArray(course);
Course decoded = mx::fromByteArray<Course>(bytes);
```

## 位域示例

C++ 位域的内存布局是实现定义的，不能直接 `memcpy` 做跨平台协议。用 `MX_BITFIELDS_U8` / `MX_BITFIELDS_U16` 按字段顺序显式打包：

- 第 0 个参数 → bit0
- 第 1 个参数 → bit1
- ……

```cpp
#include <mx/FieldSerializer.h>
#include <mx/ByteOrder.h>

struct AState
{
    unsigned char b0:1;
    unsigned char b1:1;
    unsigned char b2:1;
    unsigned char b3:1;
    unsigned char b4:1;
    unsigned char b5:1;
    unsigned char b6:1;
    unsigned char b7:1;

    MX_BITFIELDS_U8(b0, b1, b2, b3, b4, b5, b6, b7)
};

struct Device
{
    QString name;
    QList<int> nums;
    AState state;

    MX_FIELDS(name, nums, state)
    MX_BYTEODER(Device, name, nums, state)
};
```

不要对 `b0`…`b7` 使用 `MX_FIELDS` / `MX_BYTEODER`。`U8` 无字节序问题；`U16` 在 `toNetOrder` / `toHostOrder` 时会对打包后的 `uint16_t` 做端序转换。

## 构建示例与测试

```powershell
cmake -S . -B build -DMX_BUILD_EXAMPLES=ON -DMX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

若 Qt 安装在 CMake 默认搜索路径之外，请传入 `CMAKE_PREFIX_PATH`：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=D:\Qt5.12.8\5.12.8\msvc2017_64
```
