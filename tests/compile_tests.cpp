#include <mx/mx.h>

#include <QList>
#include <QString>
#include <QVector>
#include <cstdint>

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

    MX_BYTEODER(Branch, count, leaf, leaves, nestedLeaves, vectorLeaves, leafArray)
    MX_FIELDS(count, leaf, leaves, nestedLeaves, vectorLeaves)
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

    const Branch net = mx::toNetOrder(value);
    const Branch host = mx::toHostOrder(net);

    if (host.count != value.count) return 1;
    if (host.leaf.id != value.leaf.id) return 2;
    if (host.leaf.name != value.leaf.name) return 3;
    if (host.leaves.size() != value.leaves.size()) return 4;
    if (host.nestedLeaves.size() != value.nestedLeaves.size()) return 5;
    if (host.vectorLeaves.size() != value.vectorLeaves.size()) return 6;
    if (host.leafArray[1].id != value.leafArray[1].id) return 7;

    const QByteArray bytes = mx::toByteArray(value);
    const Branch decoded = mx::fromByteArray<Branch>(bytes);
    if (decoded.count != value.count) return 8;
    if (decoded.leaves[0].name != value.leaves[0].name) return 9;

    if (mx::BCD2Dec(static_cast<uint16_t>(0x1234)) != 1234) return 10;
    if (mx::Dec2BCD(1234) != 0x1234) return 11;

    return 0;
}
