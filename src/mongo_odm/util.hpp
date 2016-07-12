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

template <typename A, typename B>
struct select_non_void {
    using type = A;
};

template <typename B>
struct select_non_void<void, B> {
    using type = B;
};

/**
 * A type trait struct for determining whether a variadic list of boolean conditions is
 * all true.
 */
template <bool...>
struct bool_pack;
template <bool... bs>
struct all_true : public std::is_same<bool_pack<bs..., true>, bool_pack<true, bs...>> {};

/**
 * A type trait struct that contains true if the type T is an optional containing an arithmetic
 * type.
 */
template <typename T>
struct is_arithmetic_optional : public std::false_type {};

template <typename T>
struct is_arithmetic_optional<bsoncxx::stdx::optional<T>> : public std::is_arithmetic<T> {};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
