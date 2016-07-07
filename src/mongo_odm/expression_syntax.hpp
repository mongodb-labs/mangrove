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

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <bsoncxx/types.hpp>

#include <mongo_odm/macros.hpp>
#include <mongo_odm/util.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

// Enum of possible expression types.
enum struct expression_category { none, query, update, sort };

// Forward declarations for Expression type trait structs
template <expression_category, typename>
struct is_expression_type;

/* Type traits structs for various expressions */
template <expression_category expr_type>
struct expression_category_t {
    static constexpr expression_category value = expr_type;
};

/* forward declarations of expression classes */

template <typename NvpT>
class sort_expr;

template <expression_category list_type, typename... Args>
class expression_list;

template <typename NvpT, typename U>
class comparison_expr;

template <typename NvpT, typename U>
class comparison_value_expr;

template <typename NvpT>
class mod_expr;

template <typename Expr>
class not_expr;

class text_search_expr;

template <typename Expr1, typename Expr2>
class boolean_expr;

template <typename List>
class boolean_list_expr;

template <typename NvpT, typename U>
class update_expr;

template <typename NvpT, typename U>
class update_value_expr;

template <typename NvpT>
class unset_expr;

template <typename NvpT>
class current_date_expr;

template <typename NvpT, typename U>
class add_to_set_update_expr;

template <typename NvpT, typename U, typename Sort = int>
class push_update_expr;

template <typename NvpT, typename Integer>
class bit_update_expr;

namespace details {

/* type traits for classifying expressions by their category. */

using expression_none_t = expression_category_t<expression_category::none>;

using expression_sort_t = expression_category_t<expression_category::sort>;

using expression_query_t = expression_category_t<expression_category::query>;

using expression_update_t = expression_category_t<expression_category::update>;

// Type trait containing the expression category of a given type.

template <typename>
struct expression_type : public expression_none_t {};

template <typename T>
constexpr expression_category expression_type_v = expression_type<T>::value;

template <typename NvpT>
struct expression_type<sort_expr<NvpT>> : public expression_sort_t {};

template <expression_category list_type, typename... Args>
struct expression_type<expression_list<list_type, Args...>>
    : public expression_category_t<list_type> {};

template <typename NvpT, typename U>
struct expression_type<comparison_expr<NvpT, U>> : public expression_query_t {};

template <typename NvpT, typename U>
struct expression_type<comparison_value_expr<NvpT, U>> : public expression_query_t {};

template <typename NvpT>
struct expression_type<mod_expr<NvpT>> : public expression_query_t {};

template <typename Expr>
struct expression_type<not_expr<Expr>> : public expression_query_t {};

template <>
struct expression_type<text_search_expr> : public expression_query_t {};

template <typename Expr1, typename Expr2>
struct expression_type<boolean_expr<Expr1, Expr2>> : public expression_query_t {};

template <typename List>
struct expression_type<boolean_list_expr<List>> : public expression_query_t {};

template <typename NvpT, typename U>
struct expression_type<update_expr<NvpT, U>> : public expression_update_t {};

template <typename NvpT, typename U>
struct expression_type<update_value_expr<NvpT, U>> : public expression_update_t {};

template <typename NvpT>
struct expression_type<unset_expr<NvpT>> : public expression_update_t {};

template <typename NvpT>
struct expression_type<current_date_expr<NvpT>> : public expression_update_t {};

template <typename NvpT, typename U>
struct expression_type<add_to_set_update_expr<NvpT, U>> : public expression_update_t {};

template <typename NvpT, typename U, typename Sort>
struct expression_type<push_update_expr<NvpT, U, Sort>> : public expression_update_t {};

template <typename NvpT, typename Integer>
struct expression_type<bit_update_expr<NvpT, Integer>> : public expression_update_t {};

// type trait struct for checking if a type has a certain expression category.
template <expression_category type, typename T>
struct is_expression_type : public std::integral_constant<bool, type == expression_type_v<T>> {};

template <typename T>
struct isnt_expression : public is_expression_type<expression_category::none, T> {};

template <typename T>
constexpr bool isnt_expression_v = isnt_expression<T>::value;

template <typename T>
struct is_sort_expression : public is_expression_type<expression_category::sort, T> {};

template <typename T>
constexpr bool is_sort_expression_v = is_sort_expression<T>::value;

template <typename T>
struct is_query_expression : public is_expression_type<expression_category::query, T> {};

template <typename T>
constexpr bool is_query_expression_v = is_query_expression<T>::value;

template <typename T>
struct is_update_expression : public is_expression_type<expression_category::update, T> {};

template <typename T>
constexpr bool is_update_expression_v = is_update_expression<T>::value;

}  // namespace details
MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
