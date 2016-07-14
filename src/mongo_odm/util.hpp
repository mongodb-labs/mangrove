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

#include <type_traits>

#include <bsoncxx/stdx/optional.hpp>

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

/**
 * A type trait struct for determining whether a type is an iterable container type. This include
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

template <typename T>
std::false_type is_iterable_impl(...);

template <typename T>
using is_iterable = decltype(is_iterable_impl<T>(0));

/**
 * A type trait struct for determining whether a type is an optional.
 */
template <typename T>
struct is_optional : public std::false_type {};

template <typename T>
struct is_optional<bsoncxx::stdx::optional<T>> : public std::true_type {};

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

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
