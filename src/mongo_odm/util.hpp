// Copyright 2016 MongoDB Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <mongo_odm/config/prelude.hpp>

#include <chrono>
#include <ctime>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/types.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

template <bool...>
struct bool_pack;

/**
 * A templated struct for determining whether a variadic list of boolean conditions is all true.
 * i.e. a logical AND of a set of conditions.
 */
template <bool... bs>
struct all_true : public std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>> {};

/**
 * A type trait struct for determining whether a type is a string or C string.
 */
template <typename S>
struct is_string
    : std::integral_constant<
          bool, std::is_same<char *, typename std::decay<S>::type>::value ||
                    std::is_same<const char *, typename std::decay<S>::type>::value ||
                    std::is_same<wchar_t *, typename std::decay<S>::type>::value ||
                    std::is_same<const wchar_t *, typename std::decay<S>::type>::value> {};

template <typename Char, typename Traits, typename Allocator>
struct is_string<std::basic_string<Char, Traits, Allocator>> : std::true_type {};

template <typename S>
constexpr bool is_string_v = is_string<S>::value;

/**
 * A type trait for determining whether a type is an iterable container type. This include
 * any type that can be used with std::begin() and std::end().
 */

// To allow ADL with custom begin/end
using std::begin;
using std::end;

template <typename T>
auto is_iterable_impl(int)
    -> decltype(begin(std::declval<T &>()) !=
                    end(std::declval<T &>()),  // begin/end and operator !=
                void(),                        // Handle evil operator ,
                ++std::declval<decltype(begin(std::declval<T &>())) &>(),  // operator ++
                void(*begin(std::declval<T &>())),                         // operator*
                std::true_type{});

template <typename>
std::false_type is_iterable_impl(...);

template <typename T>
using is_iterable = decltype(is_iterable_impl<T>(0));

template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

// Matches iterables, but NOT strings or char arrays.
template <typename T>
using is_iterable_not_string = std::integral_constant<int, is_iterable_v<T> && !is_string_v<T>>;

template <typename T>
constexpr bool is_iterable_not_string_v = is_iterable_not_string<T>::value;

/**
 * A templated function whose return type is the underlying value type of a given container.
 * If the given type parameter is not a container, then the function simply returns that type
 * unchanged.
 */
template <typename T>
typename T::iterator::value_type iterable_value_impl(int);

template <typename T>
T iterable_value_impl(...);

template <typename T>
using iterable_value_t = decltype(iterable_value_impl<T>(0));

/**
 * A type trait struct for determining whether a type is an optional.
 */
template <typename T>
struct is_optional : public std::false_type {};

template <typename T>
struct is_optional<bsoncxx::stdx::optional<T>> : public std::true_type {};

template <typename T>
constexpr bool is_optional_v = is_optional<T>::value;

/**
 * A templated struct that contains its templated type, but with optionals unwrapped.
 * So `remove_optional<optional<int>>::type` is defined as `int`.
 */
template <typename T>
struct remove_optional {
    using type = T;
};

template <typename T>
struct remove_optional<bsoncxx::stdx::optional<T>> {
    using type = T;
};

template <typename T>
using remove_optional_t = typename remove_optional<T>::type;

constexpr std::int64_t bit_positions_to_mask() {
    return 0;
}

template <typename... Args>
constexpr std::int64_t bit_positions_to_mask(std::int64_t pos, Args... positions) {
    pos<0 || pos> 63 ? throw std::logic_error("Invalid pos") : 0;
    return (1 << pos) | bit_positions_to_mask(positions...);
}

/**
 * A type traits struct that determines whether a certain type stores a date. This includes <chrono>
 * time types, and the BSON b_date type. time_t is not included due to potential problems with
 * conversion to BSON.
 */

template <typename T>
struct is_date : public std::false_type {};

template <typename Rep, typename Period>
struct is_date<std::chrono::duration<Rep, Period>> : public std::true_type {};

template <typename Clock, typename Duration>
struct is_date<std::chrono::time_point<Clock, Duration>> : public std::true_type {};

template <>
struct is_date<bsoncxx::types::b_date> : public std::true_type {};

template <typename T>
constexpr bool is_date_v = is_date<T>::value;

/**
 * Method for passing each element in a tuple to a callback function.
 */
template <typename Map, typename... Ts, size_t... idxs>
constexpr void tuple_for_each_impl(const std::tuple<Ts...> &tup, Map &&map,
                                   std::index_sequence<idxs...>) {
    (void)std::initializer_list<int>{(map(std::get<idxs>(tup)), 0)...};
}

template <typename Map, typename... Ts>
constexpr void tuple_for_each(const std::tuple<Ts...> &tup, Map &&map) {
    return tuple_for_each_impl(tup, std::forward<Map>(map), std::index_sequence_for<Ts...>());
}

/**
 * Helper type trait widget that helps properly forward arguments to _id constructor in
 * mongo_odm::model. first_two_types_are_same<T1, T2, ...>::value will be true when T1 and T2 are of
 * the same type, and false otherwise.
 */
template <typename... Ts>
struct first_two_types_are_same : public std::false_type {};

template <typename T, typename T2, typename... Ts>
struct first_two_types_are_same<T, T2, Ts...> : public std::is_same<T, std::decay_t<T2>> {};

/**
 * Type trait that checks whether or not a type is a container that contains a particular type.
 *
 * @tparam container_type The container being checked for containing a particular type.
 * @tparam T The type that the container must hold for container_of::value to be true.
 */
template <typename container_type, typename T>
struct container_of : public std::is_same<typename container_type::value_type, T> {};

template <typename container_type, typename T>
constexpr bool container_of_v = container_of<container_type, T>::value;

/**
 * Type trait that checks whether or not an iterator iterates a particular type.
 *
 * @tparam iterator_type The iterator being checked for iterating a particular type.
 * @tparam T The type that the iterator must iterate for iterator_of::value to be true.
 */
template <typename iterator_type, typename T>
struct iterator_of
    : public std::is_same<typename std::iterator_traits<iterator_type>::value_type, T> {};

template <typename iterator_type, typename T>
constexpr bool iterator_of_v = iterator_of<iterator_type, T>::value;

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
