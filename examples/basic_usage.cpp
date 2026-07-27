#include <mx/ByteOrder.h>
#include <mx/FieldSerializer.h>
#include <mx/bytes_convert.h>

#include <QList>
#include <QString>
#include <cstdint>

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

    MX_BYTEODER(Packet, sequence, items)
    MX_FIELDS(sequence, items)
};

int main()
{
    Packet packet;
    packet.sequence = 0x11223344;
    packet.items.append(Item{0x1234, QStringLiteral("alpha")});

    const Packet net = MX::ByteOrder::toNetOrder(packet);
    const Packet host = MX::ByteOrder::toHostOrder(net);

    const QByteArray bytes = MX::toByteArray(host);
    const Packet decoded = MX::fromByteArray<Packet>(bytes);

    if (decoded.sequence != packet.sequence) return 1;
    if (decoded.items.size() != packet.items.size()) return 2;
    if (decoded.items[0].id != packet.items[0].id) return 3;
    if (decoded.items[0].name != packet.items[0].name) return 4;
    if (TQ::fromNet(TQ::toNet(static_cast<uint32_t>(0x12345678))) != 0x12345678) return 5;

    return 0;
}
