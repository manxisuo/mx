#pragma once

#include <QObject>
#include <QEventLoop>
#include <tuple>
#include <optional>
#include <type_traits>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QSharedPointer>
#include <QMutex>

// Signal 等待与组合工具类，对外公开 API
class SignalUtil
{
public:
    template <typename Func>
    static bool invokeQueued(typename QtPrivate::FunctionPointer<Func>::Object *object, Func function)
    {
        return QMetaObject::invokeMethod(object, function, Qt::QueuedConnection);
    }

    static void wait(int msec)
    {
        QEventLoop loop;
        QTimer::singleShot(msec, &loop, &QEventLoop::quit);
        loop.exec();
    }

    template <typename Func>
    static inline void waitSignal(const typename QtPrivate::FunctionPointer<Func>::Object *sender, Func signal)
    {
        QEventLoop loop;
        QObject::connect(sender, signal, &loop, &QEventLoop::quit);
        loop.exec();
    }

    template <typename Sender, typename Func, typename... Args>
    static std::optional<std::tuple<typename std::decay<Args>::type...>>
    waitSignalWithResult(const Sender* sender, void (Func::*signal)(Args...), int timeoutMs = 3000)
    {
        static_assert(std::is_base_of<QObject, Sender>::value, "Sender must inherit QObject");

        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);

        std::optional<std::tuple<typename std::decay<Args>::type...>> result;

        QObject::connect(sender, signal, [&loop, &result](Args... args) {
            result.emplace(std::make_tuple(args...));
            loop.quit();
        });

        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

        timer.start(timeoutMs);
        loop.exec();

        return result;
    }

    template <typename Sender, typename Signal>
    static void waitAllSignals(const QVector<QPair<Sender*, Signal>> &signalList)
    {
        QEventLoop loop;
        int total = signalList.size();
        int triggered = 0;

        QVector<QMetaObject::Connection> connections;

        for (const auto &[sender, signal] : signalList)
        {
            auto connection = QObject::connect(sender, signal, [&triggered, total, &loop]() mutable {
                triggered++;
                if (triggered >= total)
                {
                    loop.quit();
                }
            });
            connections.append(connection);
        }

        loop.exec();

        for (const auto &conn : connections)
        {
            QObject::disconnect(conn);
        }
    }

    template <typename Sender, typename Signal>
    static void waitAllSignals(std::initializer_list<QPair<Sender*, Signal>> signalList)
    {
        waitAllSignals(QVector<QPair<Sender*, Signal>>(signalList));
    }

    template <typename... Pairs>
    static void waitAllSignals(Pairs... pairs)
    {
        QEventLoop loop;
        QSharedPointer<int> counter(new int(sizeof...(Pairs)));
        QSharedPointer<QVector<QMetaObject::Connection>> connections(new QVector<QMetaObject::Connection>());

        auto connectOne = [&](auto senderSignalPair) {
            auto sender = senderSignalPair.first;
            auto signal = senderSignalPair.second;

            auto connection = QObject::connect(sender, signal, [&]() {
                (*counter)--;
                if (*counter == 0)
                {
                    foreach (const auto &conn, *connections)
                    {
                        QObject::disconnect(conn);
                    }
                    loop.quit();
                }
            });
            connections->append(connection);
        };

        int dummy[] = { (connectOne(pairs), 0)... };
        Q_UNUSED(dummy);

        loop.exec();
    }

    template <typename... Pairs>
    static void waitAnySignals(Pairs... pairs)
    {
        QEventLoop loop;
        QSharedPointer<QVector<QMetaObject::Connection>> connections(new QVector<QMetaObject::Connection>());

        auto connectOne = [&](auto senderSignalPair)
        {
            auto sender = senderSignalPair.first;
            auto signal = senderSignalPair.second;

            auto connection = QObject::connect(sender, signal, [&]() {
                foreach (const auto &conn, *connections)
                    QObject::disconnect(conn);
                loop.quit();
            });
            connections->append(connection);
        };

        int dummy[] = { (connectOne(pairs), 0)... };
        Q_UNUSED(dummy);

        loop.exec();
    }
};


