#pragma once

#include <QtCore>
#include <QtCore/private/qfutureinterface_p.h>
#include <tuple>
#include <type_traits>
#include <utility>

// ---------- 内部：类型工具 ----------
namespace mx {
namespace detail {

// 如果信号无参 → void；1 参 → 该类型（值类型）；多参 → tuple<值类型...>
template <typename... Args>
struct DefaultResult {
    using type = std::conditional_t<
        (sizeof...(Args) == 0),
        void,
        std::conditional_t<
            (sizeof...(Args) == 1),
            std::decay_t<typename std::tuple_element<0, std::tuple<Args...>>::type>,
            std::tuple<std::decay_t<Args>...>
            >
        >;
};

// C++17：推导 transform(Args...) 的返回类型
template <typename F, typename... Args>
using invoke_result_t = std::invoke_result_t<F, Args...>;

} // namespace detail

// -------------------------------------------------------------
// 1) 无 transform 版本：结果 = 默认策略（void / 单参值 / tuple<值...>）
// -------------------------------------------------------------
template <typename Sender, typename... SigArgs>
auto whenSignal(
    Sender* sender,
    void (Sender::*signal)(SigArgs...),
    QObject* context = nullptr,
    int timeoutMs = -1,                             // <0 不超时
    Qt::ConnectionType ctype = Qt::AutoConnection
    ) -> QFuture<typename detail::DefaultResult<SigArgs...>::type>
{
    using R = typename detail::DefaultResult<SigArgs...>::type;

    auto state = std::make_shared<QFutureInterface<R>>();
    state->reportStarted();
    QFuture<R> fut = state->future();

    QPointer<Sender> sp = sender;
    QPointer<QObject> cp = context;

    auto finishOnce = [state]() {
        state->reportFinished();
    };

    QMetaObject::Connection sigConn = QObject::connect(
        sender, signal,
        (context ? context : sender),
        [state, sp, finishOnce](SigArgs... args) mutable {
            if (!sp)
            {
                state->reportCanceled();
                finishOnce();
                return;
            }
            if constexpr (std::is_void<R>::value)
            {
                finishOnce();
            }
            else if constexpr (sizeof...(SigArgs) == 1)
            {
                // 单参，直接 decay 成值类型
                state->reportResult(R(std::forward<SigArgs>(args)...));
                finishOnce();
            }
            else
            {
                // 多参，打包成 tuple<decay_t<...>>
                state->reportResult(R{std::forward<SigArgs>(args)...});
                finishOnce();
            }
        },
        ctype
        );

    QMetaObject::Connection dieConn = QObject::connect(
        sender, &QObject::destroyed,
        (context ? context : sender),
        [state, finishOnce] {
            state->reportCanceled();
            finishOnce();
        },
        Qt::QueuedConnection
        );

    QMetaObject::Connection ctxConn;
    if (cp)
    {
        ctxConn = QObject::connect(cp, &QObject::destroyed,
            (context ? context : sender),
            [state, finishOnce] {
                state->reportCanceled();
                finishOnce();
            },
            Qt::QueuedConnection
            );
    }

    QPointer<QTimer> toTimer;
    if (timeoutMs >= 0)
    {
        toTimer = new QTimer(context ? context : sender);
        toTimer->setSingleShot(true);
        QObject::connect(toTimer, &QTimer::timeout,
                         (context ? context : sender),
                         [state, finishOnce] {
                             state->reportCanceled();
                             finishOnce();
                         });
        toTimer->start(timeoutMs);
    }

    auto* watcher = new QFutureWatcher<R>(context ? context : sender);
    QObject::connect(watcher, &QFutureWatcher<R>::finished, watcher, [=] {
        QObject::disconnect(sigConn);
        QObject::disconnect(dieConn);
        if (cp) QObject::disconnect(ctxConn);
        if (toTimer) toTimer->deleteLater();
        watcher->deleteLater();
    });
    watcher->setFuture(fut);

    return fut;
}

// -------------------------------------------------------------
// 2) transform 版本：结果 = transform(信号参数...)
// -------------------------------------------------------------
template <typename Sender, typename Transform, typename... SigArgs>
auto whenSignalT(
    Sender* sender,
    void (Sender::*signal)(SigArgs...),
    QObject* context,
    Transform transform,                            // (SigArgs...) -> R
    int timeoutMs = -1,
    Qt::ConnectionType ctype = Qt::AutoConnection
    ) -> QFuture<detail::invoke_result_t<Transform, SigArgs...>>
{
    using R = detail::invoke_result_t<Transform, SigArgs...>;

    auto state = std::make_shared<QFutureInterface<R>>();
    state->reportStarted();
    QFuture<R> fut = state->future();

    QPointer<Sender> sp = sender;
    QPointer<QObject> cp = context;

    auto finishOnce = [state]() {
        state->reportFinished();
    };

    QMetaObject::Connection sigConn = QObject::connect(
        sender, signal,
        (context ? context : sender),
        [state, sp, transform = std::move(transform), finishOnce](SigArgs... args) mutable {
            if (!sp)
            {
                state->reportCanceled();
                finishOnce();
                return;
            }
            if constexpr (std::is_void<R>::value)
            {
                transform(std::forward<SigArgs>(args)...);
                finishOnce();
            }
            else
            {
                state->reportResult(transform(std::forward<SigArgs>(args)...));
                finishOnce();
            }
        },
        ctype
        );

    QMetaObject::Connection dieConn = QObject::connect(
        sender, &QObject::destroyed,
        (context ? context : sender),
        [state, finishOnce] { state->reportCanceled(); finishOnce(); },
        Qt::QueuedConnection
        );

    QMetaObject::Connection ctxConn;
    if (cp)
    {
        ctxConn = QObject::connect(cp, &QObject::destroyed,
            (context ? context : sender),
            [state, finishOnce] { state->reportCanceled(); finishOnce(); },
            Qt::QueuedConnection
            );
    }

    QPointer<QTimer> toTimer;
    if (timeoutMs >= 0) {
        toTimer = new QTimer(context ? context : sender);
        toTimer->setSingleShot(true);
        QObject::connect(toTimer, &QTimer::timeout,
                         (context ? context : sender),
                         [state, finishOnce] {
                             state->reportCanceled();
                             finishOnce();
                         });
        toTimer->start(timeoutMs);
    }

    auto* watcher = new QFutureWatcher<R>(context ? context : sender);
    QObject::connect(watcher, &QFutureWatcher<R>::finished, watcher, [=] {
        QObject::disconnect(sigConn);
        QObject::disconnect(dieConn);
        if (cp) QObject::disconnect(ctxConn);
        if (toTimer) toTimer->deleteLater();
        watcher->deleteLater();
    });
    watcher->setFuture(fut);

    return fut;
}

} // namespace mx


