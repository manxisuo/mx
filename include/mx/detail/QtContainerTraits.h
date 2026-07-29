#pragma once

#include <QList>
#include <QVector>
#include <type_traits>

namespace mx {

#ifndef MX_QT_CONTAINER_TRAITS_DEFINED
#define MX_QT_CONTAINER_TRAITS_DEFINED

template<typename T>
struct is_q_list : std::false_type {};

template<typename U>
struct is_q_list<QList<U>> : std::true_type {
    using value_type = U;
};

template<typename T>
struct is_q_vector : std::false_type {};

template<typename U>
struct is_q_vector<QVector<U>> : std::true_type {
    using value_type = U;
};

#endif // MX_QT_CONTAINER_TRAITS_DEFINED

} // namespace mx
