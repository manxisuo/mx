#include <mx/ByteOrder.h>
#include <mx/FieldSerializer.h>
#include <mx/bytes_convert.h>

#include <QList>
#include <QString>
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

struct Item
{
    uint16_t id{};
    QString name;

    MX_BYTEODER(Item, id, name)
    MX_FIELDS(id, name)
};

struct Packet
{
    uint32_t sequence{};
    QList<Item> items;
    AState state;

    MX_BYTEODER(Packet, sequence, items, state)
    MX_FIELDS(sequence, items, state)
};

int main()
{
    Packet packet;
    packet.sequence = 0x11223344;
    packet.items.append(Item{0x1234, QStringLiteral("alpha")});
    packet.state.b0 = 1;
    packet.state.b1 = 0;
    packet.state.b2 = 0;
    packet.state.b3 = 0;
    packet.state.b4 = 0;
    packet.state.b5 = 0;
    packet.state.b6 = 0;
    packet.state.b7 = 1;

    const Packet net = mx::toNetOrder(packet);
    const Packet host = mx::toHostOrder(net);

    const QByteArray bytes = mx::toByteArray(host);
    const Packet decoded = mx::fromByteArray<Packet>(bytes);

    if (decoded.sequence != packet.sequence) return 1;
    if (decoded.items.size() != packet.items.size()) return 2;
    if (decoded.items[0].id != packet.items[0].id) return 3;
    if (decoded.items[0].name != packet.items[0].name) return 4;
    if (mx::fromNet(mx::toNet(static_cast<uint32_t>(0x12345678))) != 0x12345678) return 5;
    if (decoded.state.mx_pack_bits() != 0x81) return 6;

    return 0;
}
