#include <mx/ByteOrder.h>
#include <mx/FieldSerializer.h>
#include <mx/bytes_convert.h>

#include <QList>
#include <QString>
#include <QVector>
#include <cstdint>

struct AState
{
    unsigned char b0 : 1;
    unsigned char b1 : 1;
    unsigned char b2 : 1;
    unsigned char b3 : 1;
    unsigned char b4 : 1;
    unsigned char b5 : 1;
    unsigned char b6 : 1;
    unsigned char b7 : 1;

    MX_BITFIELDS_U8(b0, b1, b2, b3, b4, b5, b6, b7)
};

struct Flags16
{
    unsigned short f0 : 1;
    unsigned short f1 : 1;
    unsigned short f2 : 1;
    unsigned short f3 : 1;
    unsigned short f4 : 1;
    unsigned short f5 : 1;
    unsigned short f6 : 1;
    unsigned short f7 : 1;
    unsigned short f8 : 1;
    unsigned short f9 : 1;
    unsigned short f10 : 1;
    unsigned short f11 : 1;
    unsigned short f12 : 1;
    unsigned short f13 : 1;
    unsigned short f14 : 1;
    unsigned short f15 : 1;

    MX_BITFIELDS_U16(f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15)
};

struct Leaf
{
    uint16_t id{};
    QString name;

    MX_BYTEODER(Leaf, id, name)
    MX_FIELDS(id, name)
};

struct Branch
{
    uint32_t count{};
    Leaf leaf;
    QList<Leaf> leaves;
    QList<QList<Leaf>> nestedLeaves;
    QVector<Leaf> vectorLeaves;
    Leaf leafArray[2];
    AState state;
    Flags16 flags;

    MX_BYTEODER(Branch, count, leaf, leaves, nestedLeaves, vectorLeaves, leafArray, state, flags)
    MX_FIELDS(count, leaf, leaves, nestedLeaves, vectorLeaves, state, flags)
};

int main()
{
    Branch value;
    value.count = 0x11223344;
    value.leaf.id = 0x1234;
    value.leaf.name = QStringLiteral("leaf");
    value.leaves.append(value.leaf);
    value.nestedLeaves.append(value.leaves);
    value.vectorLeaves.append(value.leaf);
    value.leafArray[0] = value.leaf;
    value.leafArray[1].id = 0x5678;
    value.state.b0 = 1;
    value.state.b1 = 0;
    value.state.b2 = 0;
    value.state.b3 = 1;
    value.state.b4 = 0;
    value.state.b5 = 0;
    value.state.b6 = 0;
    value.state.b7 = 1;
    value.flags.f0 = 1;
    value.flags.f1 = 0;
    value.flags.f2 = 0;
    value.flags.f3 = 0;
    value.flags.f4 = 0;
    value.flags.f5 = 0;
    value.flags.f6 = 0;
    value.flags.f7 = 0;
    value.flags.f8 = 1;
    value.flags.f9 = 0;
    value.flags.f10 = 0;
    value.flags.f11 = 0;
    value.flags.f12 = 0;
    value.flags.f13 = 0;
    value.flags.f14 = 0;
    value.flags.f15 = 1;

    const Branch net = mx::toNetOrder(value);
    const Branch host = mx::toHostOrder(net);

    if (host.count != value.count) return 1;
    if (host.leaf.id != value.leaf.id) return 2;
    if (host.leaf.name != value.leaf.name) return 3;
    if (host.leaves.size() != value.leaves.size()) return 4;
    if (host.nestedLeaves.size() != value.nestedLeaves.size()) return 5;
    if (host.vectorLeaves.size() != value.vectorLeaves.size()) return 6;
    if (host.leafArray[1].id != value.leafArray[1].id) return 7;
    if (host.state.b0 != 1 || host.state.b3 != 1 || host.state.b7 != 1) return 12;
    if (host.flags.f0 != 1 || host.flags.f8 != 1 || host.flags.f15 != 1) return 13;

    const QByteArray bytes = mx::toByteArray(value);
    const Branch decoded = mx::fromByteArray<Branch>(bytes);
    if (decoded.count != value.count) return 8;
    if (decoded.leaves[0].name != value.leaves[0].name) return 9;
    if (decoded.state.mx_pack_bits() != value.state.mx_pack_bits()) return 14;
    if (decoded.flags.mx_pack_bits() != value.flags.mx_pack_bits()) return 15;

    // 协议布局：AState 固定写 1 字节，Flags16 固定写 2 字节（宿主字节序下的打包整数）
    AState alone{};
    alone.b0 = 1;
    alone.b1 = 1;
    // 其余位保持 0，避免未初始化位域影响 pack 结果
    alone.b2 = 0;
    alone.b3 = 0;
    alone.b4 = 0;
    alone.b5 = 0;
    alone.b6 = 0;
    alone.b7 = 0;
    const QByteArray stateBytes = mx::toByteArray(alone);
    if (stateBytes.size() != 1) return 16;
    if (static_cast<uint8_t>(stateBytes.at(0)) != 0x03) return 17;

    Flags16 alone16{};
    alone16.f0 = 1;
    alone16.f8 = 1;
    const QByteArray flagsBytes = mx::toByteArray(alone16);
    if (flagsBytes.size() != 2) return 18;
    const uint16_t packed16 = alone16.mx_pack_bits();
    if (packed16 != 0x0101) return 19;

    if (mx::BCD2Dec(static_cast<uint16_t>(0x1234)) != 1234) return 10;
    if (mx::Dec2BCD(1234) != 0x1234) return 11;

    return 0;
}
