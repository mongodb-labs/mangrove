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
#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <bsoncxx/builder/core.hpp>
#include <bsoncxx/view_or_value.hpp>

#include <mongo_odm/expression_syntax.hpp>
#include <mongo_odm/nvp.hpp>
#include <mongo_odm/util.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

/**
 * Type trait that contains true if a type T can be appended to a BSON builder using
 * builder.append(T val);
 */
template <typename T>
auto is_bson_appendable_impl(int)
    -> decltype(std::declval<bsoncxx::builder::core>().append(std::declval<T>()), std::true_type{});

template <typename T>
std::false_type is_bson_appendable_impl(...);

template <typename T>
using is_bson_appendable = decltype(is_bson_appendable_impl<T>(0));

template <typename T>
constexpr bool is_bson_appendable_v = is_bson_appendable<T>::value;

/**
 * Templated function for appending a value to a BSON builder. If possible, the function simply
 * passes the value directly to the builder. If it cannot be nicely appended, it is first
 * serialized and then added as a sub-document to the builder. If the value is a container/iterable,
 * it is serialized into a BSON array. If the value is a query builder epxression, it is serialized
 * using its member function .append_to_bson(builder).
 */
// Specialization for appendable types.
template <typename T>
std::enable_if_t<is_bson_appendable_v<T>> append_value_to_bson(T value,
                                                               bsoncxx::builder::core &builder) {
    builder.append(value);
}

// Specialization for non-iterable, non-expression types that must be serialized.
template <typename T>
std::enable_if_t<!is_bson_appendable_v<T> && !is_iterable_not_string_v<T> &&
                 details::isnt_expression_v<T>>
append_value_to_bson(const T &value, bsoncxx::builder::core &builder) {
    auto serialized_value = bson_mapper::to_document<T>(value);
    builder.append(bsoncxx::types::b_document{serialized_value});
}

// Specialization for iterable types that must be serialized.
template <typename Iterable>
std::enable_if_t<is_iterable_not_string_v<Iterable>> append_value_to_bson(
    const Iterable &arr, bsoncxx::builder::core &builder) {
    builder.open_array();
    for (const auto &x : arr) {
        append_value_to_bson(x, builder);
    }
    builder.close_array();
}

// Specialization for expression types that must be serialized.
template <typename Expression>
std::enable_if_t<!details::isnt_expression_v<Expression>> append_value_to_bson(
    const Expression &expr, bsoncxx::builder::core &builder) {
    builder.open_document();
    expr.append_to_bson(builder);
    builder.close_document();
}

// std::chrono::time_point must be explicitly serialized into b_date
template <typename Clock, typename Duration>
void append_value_to_bson(const std::chrono::time_point<Clock, Duration> &tp,
                          bsoncxx::builder::core &builder) {
    builder.append(bsoncxx::types::b_date(tp));
}

/**
 * An expression that represents a sorting order.
 * This consists of a name-value pair and a boolean specifying ascending or descending sort
 * order. The resulting BSON is {field: +/-1}, where +/-1 corresponds to the sort order.
 */
template <typename NvpT>
class sort_expr {
   public:
    using field_type = NvpT;
    /**
     * Creates a sort expression with the given name-value-pair and sort order.
     * @param  nvp       The name-value pair to order against.
     * @param  ascending Whether to use ascending (true) or descending (false) order.
     */
    constexpr sort_expr(const NvpT &nvp, bool ascending) : _nvp(nvp), _ascending(ascending) {
    }

    /**
     * Appends this expression to a BSON core builder, as a key-value pair of the
     * form "key: +/-1".
     * @param builder   A BSON core builder
     * @param wrap      Whether to wrap the BSON inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }

        std::string s;
        builder.key_view(_nvp.append_name(s));
        builder.append(_ascending ? 1 : -1);

        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query.
     * The resulting BSON is of the form "{key: +/-1}"
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

   private:
    const NvpT _nvp;
    const bool _ascending;
};

/**
 * Represents a query expression with the syntax "key: {$op: value}".
 * This usually means queries that are comparisons, such as (User.age > 21).
 * However, this also covers operators such as $exists, or any operator that has the above
 * syntax.
 * @tparam NvpT The type of the name-value pair this expression uses.
 * @tparam U    The type of the value to compare against. This could be the same as the value
 * type
 *              of NvpT, or the type of some other parameter, or even a query builder
 *              expression.
 */
template <typename NvpT, typename U>
class comparison_expr {
   public:
    using field_type = NvpT;
    /**
     * Constructs a query expression for the given key, value, and comparison type
     * @param  nvp           A name-value pair corresponding to a key in a document
     * @param  field         The value that the key is being compared to.
     * @param  op            The type of comparison operator, such at gt (>) or ne (!=).
     */
    constexpr comparison_expr(const NvpT &nvp, const U &field, const char *op)
        : _nvp(nvp), _field(field), _operator(op) {
    }

    /**
     * Takes a comparison expression, and creates a new one with a different operator, but the
     * same value and field.
     * This is primarily used to wrap $regex operators in $not,
     * since $not cannot contains a $regex operator, it must directly contain the regex itself.
     * @param  expr          A comparison expresison with a similar field type and value type.
     * @param  op            The new operator to use.
     */
    constexpr comparison_expr(const comparison_expr &expr, const char *op)
        : _nvp(expr._nvp), _field(expr._field), _operator(op) {
    }

    /**
     * Appends the name of the contained field to a string.
     */
    std::string &append_name(std::string &s) const {
        return _nvp.append_name(s);
    }

    /**
     * Appends this expression to a BSON core builder, as a key-value pair of the form
     * "key: {$cmp: val}", where $cmp is some comparison operator.
     * @param builder   A BSON core builder
     * @param wrap      Whether to wrap the BSON inside a document.
     * @param omit_name Whether to skip the name of the field. This is used primarily in not_expr
     *                  and free_expr to append just the value of the expression.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false,
                        bool omit_name = false) const {
        if (wrap) {
            builder.open_document();
        }
        if (!omit_name && !is_free_nvp_v<field_type>) {
            std::string s;
            builder.key_view(_nvp.append_name(s));
            builder.open_document();
        }

        builder.key_view(_operator);
        append_value_to_bson(_field, builder);

        if (!omit_name && !is_free_nvp_v<field_type>) {
            builder.close_document();
        }
        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query.
     * The format of the BSON is "{key: {$not: {$cmp: val}}}".
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

   private:
    const NvpT _nvp;
    const U &_field;
    const char *_operator;
};

/**
 * Represents a comparison expression as above, but stores a value instead of a reference.
 * This is useful for storing temporary or computed values.
 * Internally, it uses a comparison_expr whose value is a b_regex.
 */
template <typename NvpT, typename U>
class comparison_value_expr : public comparison_expr<NvpT, U> {
   public:
    constexpr comparison_value_expr(NvpT nvp, const U &value, const char *op)
        : comparison_expr<NvpT, U>(nvp, _value, op), _value(value) {
    }

   private:
    const U _value;
};

/**
 * This class represents a query expression using the $mod operator, that checks the modulus of
 * a
 * certain numerical field.
 */
template <typename NvpT>
class mod_expr {
   public:
    using field_type = NvpT;
    /**
     * Constructs a mod_expr that represents a query with the $mod operator.
     * @param  nvp       The name-value pair to compare against.
     * @param  divisor   The divisor used in the modulus operation.
     * @param  remainder The remainder produced when dividing the field by the divisor.
     */
    constexpr mod_expr(NvpT nvp, const int &divisor, const int &remainder)
        : _nvp(nvp), _divisor(divisor), _remainder(remainder) {
    }

    /**
     * Appends the name of the contained field to a string.
     */
    std::string &append_name(std::string &s) const {
        return _nvp.append_name(s);
    }

    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form " key: { $mod: [ divisor, remainder ] } ".
     * @param builder   a BSON core builder
     * @param wrap      Whether to wrap the BSON inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false,
                        bool omit_name = false) const {
        if (wrap) {
            builder.open_document();
        }
        if (!omit_name && !is_free_nvp_v<field_type>) {
            std::string s;
            builder.key_view(_nvp.append_name(s));
            builder.open_document();
        }
        builder.key_view("$mod");
        builder.open_array();
        builder.append(_divisor);
        builder.append(_remainder);
        builder.close_array();
        if (!omit_name && !is_free_nvp_v<field_type>) {
            builder.close_document();
        }
        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query.
     * The resulting BSON is of the form "{ key: { $mod: [ divisor, remainder ] } }".
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

   private:
    const NvpT _nvp;
    const int &_divisor;
    const int &_remainder;
};

/**
 * Represents a query that performs a text search with the $text operator.
 * TODO check values of `language` against spec?
 */
class text_search_expr {
   public:
    /**
     * Creates a text search expression.
     * These parameters correspond to the parameters for the $text operator in MongoDB.
     * @param  search              A string of terms use to query the text index.
     * @param  language            The language of the text index.
     * @param  case_sensitive      A boolean flag to specify case-sensitive search. Optional,
     *                             false by default.
     * @param  diacritic_sensitive A boolean flag to specify case-sensitive search. Optional,
     *                             false by default.
     */
    text_search_expr(const char *search,
                     bsoncxx::stdx::optional<const char *> language = bsoncxx::stdx::nullopt,
                     bsoncxx::stdx::optional<bool> case_sensitive = bsoncxx::stdx::nullopt,
                     bsoncxx::stdx::optional<bool> diacritic_sensitive = bsoncxx::stdx::nullopt)
        : _search(search),
          _language(language),
          _case_sensitive(case_sensitive),
          _diacritic_sensitive(diacritic_sensitive) {
    }

    text_search_expr &language() {
        _language = bsoncxx::stdx::nullopt;
        return *this;
    }

    text_search_expr &language(const char *lang) {
        _language = lang;
        return *this;
    }

    text_search_expr &case_sensitive() {
        _case_sensitive = bsoncxx::stdx::nullopt;
        return *this;
    }

    text_search_expr &case_sensitive(bool case_sensitive) {
        _case_sensitive = case_sensitive;
        return *this;
    }

    text_search_expr &diacritic_sensitive() {
        _diacritic_sensitive = bsoncxx::stdx::nullopt;
        return *this;
    }

    text_search_expr &diacritic_sensitive(bool diacritic_sensitive) {
        _diacritic_sensitive = diacritic_sensitive;
        return *this;
    }

    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form " key: { $mod: [ divisor, remainder ] } ".
     * @param builder   a BSON core builder
     * @param wrap      Whether to wrap the BSON inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view("$text");
        builder.open_document();
        builder.key_view("$search");
        builder.append(_search);
        if (_language) {
            builder.key_view("$language");
            builder.append(_language.value());
        }
        // hese values are false by default, so one can save bandwidth space ommitting them unless
        // they're set to true.
        if (_case_sensitive) {
            builder.key_view("$caseSensitive");
            builder.append(_case_sensitive.value());
        }
        if (_diacritic_sensitive) {
            builder.key_view("$diacriticSensitive");
            builder.append(_diacritic_sensitive.value());
        }
        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query.
     * The resulting BSON is of the form "{ key: { $mod: [ divisor, remainder ] } }".
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

   private:
    const char *_search;
    mongocxx::stdx::optional<const char *> _language;
    bsoncxx::stdx::optional<bool> _case_sensitive;
    bsoncxx::stdx::optional<bool> _diacritic_sensitive;
};

/**
 * This represents an expression with the $not operator, which wraps a
 * comparison expression and
 * negates it.
 */
template <typename Expr>
class not_expr {
   public:
    using field_type = typename Expr::field_type;

    /**
     * Creates a $not expression that negates the given comparison expression.
     * @param  expr A comparison expression
     */
    constexpr not_expr(const Expr &expr) : _expr(expr) {
    }

    /**
     * Appends the name of the contained field to a string.
     */
    std::string &append_name(std::string &s) const {
        return _expr.append_name(s);
    }

    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form "key: {$not: {$cmp: val}}".
     * @param builder   a BSON core builder
     * @param wrap      Whether to wrap the BSON inside a document.
     * @param omit_name Whether to skip the name of the field. This is used primarily in
     *                  $elemMatch queries with scalar arrays, so one can have a query like
     *                  {array: {$elemMatch: {$not: {$gt: 5}}}}
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false,
                        bool omit_name = false) const {
        if (wrap) {
            builder.open_document();
        }
        if (!omit_name && !is_free_nvp_v<field_type>) {
            std::string s;
            builder.key_view(_expr.append_name(s));
            builder.open_document();
        }

        builder.key_view("$not");
        // Append contained expression in a document, without its name.
        _expr.append_to_bson(builder, true, true);

        if (!omit_name && !is_free_nvp_v<field_type>) {
            builder.close_document();
        }
        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query.
     * The resulting BSON is of the form "{key: {$cmp: val}}".
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

   private:
    const Expr _expr;
};

/**
 * This represents a list of expressions.
 * @tparam list_type The category of the expressions, such as "update" or "query".
 * @tparam Args...   The types of the various expressions that make up the list.
 */
template <expression_category list_type, typename... Args>
class expression_list {
   public:
    /**
     * Constructs an expression list of the given arguments.
     * @tparam args  The individual elements of the list.
     */
    expression_list(const Args &... args) : storage(std::make_tuple(args...)) {
    }

    /**
     * Appends each element to a BSON code builder.
     * @param builder A code BSON builder
     * @param wrap    Whether to wrap individual elements inside a BSON document, e.g.
     *                "{elt1...}, {elt2, ...}, ..."
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        tuple_for_each(storage, [&](const auto &v) { v.append_to_bson(builder, wrap); });
    }

    /**
     * Casts the expression list to a BSON query of  the form { expr1, expr2, ....}
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

    std::tuple<Args...> storage;
};

/**
 * This represents a boolean expression with two arguments.
 * The arguments can, in turn, be boolean expressions.
 */
template <typename Expr1, typename Expr2>
class boolean_expr {
   public:
    /**
     * Constructs a boolean expression from two other expressions, and a certain operator.
     * @param  lhs The left-hand side of the expression.
     * @param  rhs The right-hand side of the expression.
     * @param  op  The operator of the expression, e.g. AND or OR.
     */
    constexpr boolean_expr(const Expr1 &lhs, const Expr2 &rhs, const char *op)
        : _lhs(lhs), _rhs(rhs), _op(op) {
    }

    /**
     * Appends this query to a BSON core builder as a key-value pair "$op: [{lhs}, {rhs}]"
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }

        builder.key_view(_op);
        builder.open_array();

        // append left hand side
        builder.open_document();
        _lhs.append_to_bson(builder, wrap);
        builder.close_document();

        // append right hand side
        builder.open_document();
        _rhs.append_to_bson(builder, wrap);
        builder.close_document();

        builder.close_array();

        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query,
     * in the form "{ $op: [{lhs}, {rhs}] }"
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

    const Expr1 _lhs;
    const Expr2 _rhs;
    const char *_op;
};

/**
 * This class represents a boolean expression over an array of arguments.
 * This is particularly useful for the $nor operator.
 * When converted to BSON, this class produces an expression {$op: [{arg1}, {arg2}, ...]}
 */
template <typename List>
class boolean_list_expr {
   public:
    /**
     * Constructs a boolean expression from a list of sub-expressions, and a certain operator.
     * @param args An expression list of boolean conditions.
     * @param op   The operator of the expression, e.g. AND or OR.
     */
    constexpr boolean_list_expr(const List args, const char *op) : _args(args), _op(op) {
    }

    /**
     * Appends this query to a BSON core builder as a key-value pair "$op: [{lhs}, {rhs}]"
     * @param builder A basic BSON core builder.
     * @param wrap    Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_op);
        builder.open_array();
        _args.append_to_bson(builder, true);
        builder.close_array();
        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts the expression to a BSON filter for a query,
     * in the form "{$op: [{arg1}, {arg2}, ...]}"
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

    const List _args;
    const char *_op;
};

/**
 * An expression that wraps another expression and adds an $isolated operator.
 */
template <typename Expr>
class isolated_expr {
   public:
    constexpr isolated_expr(const Expr &expr) : _expr(expr) {
    }

    /**
     * Appends this query to a BSON core builder, with $isolated set as an extra field.
     *
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }

        _expr.append_to_bson(builder, false);
        builder.key_view("$isolated");
        builder.append(1);

        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Converts this query to BSON, in the form {$isolated: 1, <underlying expression BSON>}
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const Expr _expr;
};

/**
 * Represents an update operator that modifies a certain elements.
 * This creates BSON expressions of the form "$op: {field: value}",
 * where $op can be $set, $inc, etc.
 *
 * This class stores the value to be compared by reference.
 * To store a temporary or computed value, use UpdateExprValue.
 *
 * @tparam NvpT The type of the name-value pair
 * @tparam U    The type of the value being compared.
 */
template <typename NvpT, typename U>
class update_expr {
   public:
    using field_type = NvpT;
    constexpr update_expr(const NvpT &nvp, const U &val, const char *op)
        : _nvp(nvp), _val(val), _op(op) {
    }

    /**
     * Appends this query to a BSON core builder as a key-value pair "$op: {field: value}"
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_op);
        builder.open_document();
        std::string s;
        builder.key_view(_nvp.append_name(s));
        append_value_to_bson(_val, builder);
        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    /**
     * Creates a BSON value of the form "$op: {field: value}"
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const NvpT _nvp;
    const U &_val;
    const char *_op;
};

template <typename NvpT, typename U>
class update_value_expr : public update_expr<NvpT, U> {
   public:
    constexpr update_value_expr(NvpT nvp, U value, const char *op)
        : update_expr<NvpT, U>(nvp, _val, op), _val(value) {
    }

   private:
    const U _val;
};

/**
 * Represents an expresion that uses the $unset operator.
 */
template <typename NvpT>
class unset_expr {
   public:
    using field_type = NvpT;
    constexpr unset_expr(const NvpT &nvp) : _nvp(nvp) {
    }

    /**
     * Appends this query to a BSON core builder as a key-value pair "$unset: {field: ''}"
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view("$unset");
        builder.open_document();
        std::string s;
        builder.key_view(_nvp.append_name(s));
        builder.append("");
        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const NvpT _nvp;
};

/**
 * Creates an expression that uses the $currentDate operator.
 */
template <typename NvpT>
class current_date_expr {
   public:
    using field_type = NvpT;
    /**
     * Creates an expression that uses the $currentDate operator with a given field and type.
     * @param  nvp     The given field
     * @param  is_date Whether the field's type is a date (true) or a timestmap (false)
     */
    constexpr current_date_expr(const NvpT &nvp, bool is_date) : _nvp(nvp), _is_date(is_date) {
    }

    /**
     * Appends this query to a BSON core builder as an expression
     * '$currentDate: {field: {$type: "timestamp|date"}}'
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view("$currentDate");
        builder.open_document();
        std::string s;
        builder.key_view(_nvp.append_name(s));

        // type specification
        builder.open_document();
        builder.key_view("$type");
        builder.append(_is_date ? "date" : "timestamp");
        builder.close_document();

        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const NvpT _nvp;
    const bool _is_date;
};

/**
 * An expression that uses the $addToSet operator to add unique elements to an array.
 * @tparam NvpT The name-value pair to use
 * @tparam U    The type of the value to pass to $addToSet
 */
template <typename NvpT, typename U>
class add_to_set_update_expr {
   public:
    using field_type = NvpT;
    /**
     * Constructs an add_to_set_update_expr with the given parameters.
     * @param  nvp  The given field.
     * @param  val  The value to add to the field.
     * @param  each Whether to use the $each modifier.
     */
    constexpr add_to_set_update_expr(const NvpT &nvp, const U &val, bool each)
        : _nvp(nvp), _val(val), _each(each) {
    }

    /**
     * Appends this query to a BSON core builder as an expression
     * '$addToSet: {field: value | {$each: value}}'
     * @param builder A basic BSON core builder.
     * @param wrap    Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view("$addToSet");
        builder.open_document();
        std::string s;
        builder.key_view(_nvp.append_name(s));

        // wrap value in $each: {} if necessary.
        if (_each) {
            builder.open_document();
            builder.key_view("$each");
        }
        append_value_to_bson(_val, builder);
        if (_each) {
            builder.close_document();
        }

        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const NvpT _nvp;
    const U &_val;
    bool _each;
};

/**
 * Represents an array update epression that uses the $push operator.
 * Modifiers can be set either in the constructor, or by calling the corresponding member
 * functions.
 * @tparam NvpT     The name-value-pair type of the corresponding field.
 * @tparam U        The value being $push'ed to the array
 * @tparam Sort     The type of the sort expression used in the $sort modifier.
 *                  This can be either an integer, +/- 1, or a sort_expr.
 */
template <typename NvpT, typename U, typename Sort>
class push_update_expr {
   public:
    using field_type = NvpT;
    /**
     * Constructs a $push expression, with the given optional modifiers
     * @param  nvp      The field to modify
     * @param  val      The value to append to the field
     * @param  each     Whether to append a single value, or a bunch of values in an array.
     * @param  slice    An optional value to give the $slice modifier. This only takes effect
     * with
     *                  each=true.
     * @param  sort     An optional value to give the $sort modifier. This only takes effect
     *                  with each=true.
     * @param  position An optional value to give the $position modifier. This only takes effect
     *                  with each=true.
     */
    constexpr push_update_expr(
        const NvpT &nvp, const U &val, bool each,
        bsoncxx::stdx::optional<std::int32_t> slice = bsoncxx::stdx::nullopt,
        const bsoncxx::stdx::optional<Sort> &sort = bsoncxx::stdx::nullopt,
        bsoncxx::stdx::optional<std::uint32_t> position = bsoncxx::stdx::nullopt)
        : _nvp(nvp), _val(val), _each(each), _slice(slice), _sort(sort), _position(position) {
    }

    /* Functions that set new values for modifers. These create new copies of the expression,
     * primarily because $sort can take different types of parameters. */

    /**
     * Create a copy of this expression with a different $slice modifier value.
     * @param slice    The integer value of the $slice modifier.
     * @return         A new push_update_expr with the same properties as this one, except a
     *                 different $slice modifier.
     */
    constexpr push_update_expr<NvpT, U, Sort> slice(std::int32_t slice) {
        return {_nvp, _val, _each, slice, _sort, _position};
    }

    /**
     * Create a copy of this expression without a $slice modifier.
     * @return         A new push_update_expr with the same properties as this one, except a
     *                 different $slice modifier.
     */
    constexpr push_update_expr<NvpT, U, Sort> slice() {
        return {_nvp, _val, _each, bsoncxx::stdx::nullopt, _sort, _position};
    }

    /**
     * Create a copy of this expression with a different $sort modifier value.
     * @tparam OtherNvpT    The name-value-pair used by the given Sort Expression.
     * @param sort          The sort expression to use.
     * @return              A new push_update_expr with the same properties as this one, except a
     *                      different $sort modifier.
     */
    template <typename OtherNvpT>
    constexpr push_update_expr<NvpT, U, sort_expr<OtherNvpT>> sort(
        const sort_expr<OtherNvpT> &sort) {
        return {_nvp, _val, _each, _slice, sort, _position};
    }

    /**
     * Create a copy of this expression with a different $slice modifier value.
     * @param sort    The integer value of the $sort modifier, +/-1.
     * @return         A new push_update_expr with the same properties as this one, except a
     *                 different $sort modifier.
     */
    constexpr push_update_expr<NvpT, U, int> sort(int sort) {
        return {_nvp, _val, _each, _slice, sort, _position};
    }

    /**
     * Create a copy of this expression without a $sort modifier.
     * @return         A new push_update_expr with the same properties as this one, except a
     *                 different $sort modifier.
     */
    constexpr push_update_expr<NvpT, U, Sort> sort() {
        return {_nvp, _val, _each, _slice, bsoncxx::stdx::nullopt, _position};
    }

    /**
     * Create a copy of this expression with a different $position modifier value.
     * @param position    The integer value of the $position modifier.
     * @return            A new push_update_expr with the same properties as this one, except a
     *                    different $position modifier.
     */
    constexpr push_update_expr<NvpT, U, Sort> position(std::uint32_t position) {
        return {_nvp, _val, _each, _slice, _sort, position};
    }

    /**
     * Create a copy of this expression without $position modifier.
     * @return         A new push_update_expr with the same properties as this one, except a
     *                 different $position modifier.
     */
    constexpr push_update_expr<NvpT, U, Sort> position() {
        return {_nvp, _val, _each, _slice, _sort, bsoncxx::stdx::nullopt};
    }

    /**
     * Appends this query to a BSON core builder as an expression
     * '$push: {field: value | {$each: value, $modifiers: params...}}'
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view("$push");
        builder.open_document();
        std::string s;
        builder.key_view(_nvp.append_name(s));

        // wrap value in "$each: {}" if necessary.
        if (_each) {
            builder.open_document();
            builder.key_view("$each");
        }
        append_value_to_bson(_val, builder);
        if (_each) {
            if (_slice) {
                builder.key_view("$slice");
                builder.append(_slice.value());
            }
            if (_sort) {
                builder.key_view("$sort");
                append_value_to_bson(_sort.value(), builder);
            }
            if (_position) {
                builder.key_view("$position");
                builder.append(static_cast<std::int64_t>(_position.value()));
            }
            builder.close_document();
        }

        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const NvpT _nvp;
    const U &_val;
    const bool _each;
    const bsoncxx::stdx::optional<std::int32_t> _slice;
    const bsoncxx::stdx::optional<Sort> _sort;
    const bsoncxx::stdx::optional<std::uint32_t> _position;
};

/**
 * Expression that updates field using the $bit operator, which does bitwise operations using a
 * mask.
 * @tparam NvpT     The name-value-pair type corresponding to a field
 * @tparam Integer  The integral type used as a bit mask
 */
template <typename NvpT, typename Integer>
class bit_update_expr {
   public:
    using field_type = NvpT;
    constexpr bit_update_expr(const NvpT &nvp, Integer mask, const char *op)
        : _nvp(nvp), _mask(mask), _operation(op){};

    /**
     * Appends this query to a BSON core builder as an expression
     * '$bit: { <field>: { <and|or|xor>: <int> } }'
     * @param builder A basic BSON core builder.
     * @param Whether to wrap this expression inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view("$bit");
        builder.open_document();
        std::string s;
        builder.key_view(_nvp.append_name(s));

        // bit operation
        builder.open_document();
        builder.key_view(_operation);
        builder.append(_mask);
        builder.close_document();

        builder.close_document();
        if (wrap) {
            builder.close_document();
        }
    }

    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return {builder.extract_document()};
    }

   private:
    const NvpT _nvp;
    const Integer _mask;
    const char *_operation;
};

/* Query comparison operators */

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> eq(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return {lhs, rhs, "$eq"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> operator==(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return eq(lhs, rhs);
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> gt(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return {lhs, rhs, "$gt"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> operator>(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return gt(lhs, rhs);
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> gte(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return {lhs, rhs, "$gte"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> operator>=(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return gte(lhs, rhs);
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> lt(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return {lhs, rhs, "$lt"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> operator<(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return lt(lhs, rhs);
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> lte(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return {lhs, rhs, "$lte"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> operator<=(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return lte(lhs, rhs);
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> ne(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return {lhs, rhs, "$ne"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, typename NvpT::no_opt_type> operator!=(
    const NvpT &lhs, const typename NvpT::no_opt_type &rhs) {
    return ne(lhs, rhs);
}

/**
 * Negates a comparison expression in a $not expression.
 */
// TODO replace is_query_expression with is_unary_expression or something.
template <typename Expr, typename = std::enable_if_t<details::is_query_expression_v<Expr>>>
constexpr not_expr<Expr> operator!(const Expr &expr) {
    return {expr};
}

// Specialization of the ! operator for regexes, since the $regex operator cannot appear inside
// a $not operator.
// Instead, create an expression of the form {field: {$not: /regex/}}
template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>>
constexpr comparison_expr<NvpT, bsoncxx::types::b_regex> operator!(
    const comparison_value_expr<NvpT, bsoncxx::types::b_regex> &regex_expr) {
    return {regex_expr, "$not"};
}

template <typename Expr, expression_category list_type, typename... Args, size_t... idxs>
constexpr expression_list<list_type, Expr, Args...> append_impl(
    expression_list<list_type, Args...> list, Expr expr, std::index_sequence<idxs...>) {
    return {expr, std::get<idxs>(list.storage)...};
}

/**
 * Comma operator that combines two expressions or an expression and an expression list into a
 * new expression list.
 */
template <typename Expr, expression_category list_type,
          typename = std::enable_if_t<details::is_expression_type<list_type, Expr>::value>,
          typename... Args>
constexpr expression_list<list_type, Expr, Args...> operator,(
    expression_list<list_type, Args...> &&list, Expr &&expr) {
    return append_impl(list, expr, std::index_sequence_for<Args...>());
}

template <typename Expr1, typename Expr2,
          typename = std::enable_if_t<!details::isnt_expression_v<Expr1> &&
                                      (details::expression_type<Expr1>::value ==
                                       details::expression_type<Expr2>::value)>>
constexpr expression_list<details::expression_type<Expr1>::value, Expr1, Expr2> operator,(
    Expr1 &&expr1, Expr2 &&expr2) {
    return {expr1, expr2};
}

/**
 * Boolean operator overloads for expressions.
 */
template <typename Expr1, typename Expr2,
          typename = std::enable_if_t<details::is_query_expression_v<Expr1> &&
                                      details::is_query_expression_v<Expr2>>>
constexpr boolean_expr<Expr1, Expr2> operator&&(const Expr1 &lhs, const Expr2 &rhs) {
    return {lhs, rhs, "$and"};
}

template <typename Expr1, typename Expr2,
          typename = std::enable_if_t<details::is_query_expression_v<Expr1> &&
                                      details::is_query_expression_v<Expr2>>>
constexpr boolean_expr<Expr1, Expr2> operator||(const Expr1 &lhs, const Expr2 &rhs) {
    return {lhs, rhs, "$or"};
}

// TODO I'd like list ops for "and" and "or" as well, but these are reserved keywords.
// Perhaps use list_and, list_or, list_nor instead?
// Currently chaining &&'s or ||'s provides the same results as a boolean operation on a list,
// and
// not much different in terms of performance.

/**
 * A function that creates a $nor operator out of an expression_list containing arguments
 * @param list The list of arguments to the $nor operator, as an expression list.
 * @return A boolean_llist_expr that wraps the given list.
 */
template <typename... Args>
constexpr boolean_list_expr<expression_list<expression_category::query, Args...>> nor(
    const expression_list<expression_category::query, Args...> &list) {
    return {list, "$nor"};
}

/**
 * Function that creates a $nor operator out of a list of arguments.
 */
template <typename... QueryExpressions,
          typename = typename all_true<details::is_query_expression_v<QueryExpressions>...>::type>
constexpr boolean_list_expr<expression_list<expression_category::query, QueryExpressions...>> nor(
    QueryExpressions... args) {
    return {{args...}, "$nor"};
}

/**
* Creates a text search expression.
* These parameters correspond to the parameters for the $text operator in MongoDB.
* Optional parameters can be passed as optional<> types here, or with "fluent" setters on the
* resulting text_search_expr object.
* @param  search              A string of terms use to query the text index.
* @param  language            The language of the text index.
* @param  case_sensitive      A boolean flag to specify case-sensitive search. Optional, false
*                             by default.
* @param  diacritic_sensitive A boolean flag to specify case-sensitive search. Optional, false
*                             by default.
* @return                     A text_search_expr with the given parameters.
*/
inline text_search_expr text(
    const char *search, bsoncxx::stdx::optional<const char *> language = bsoncxx::stdx::nullopt,
    bsoncxx::stdx::optional<bool> case_sensitive = bsoncxx::stdx::nullopt,
    bsoncxx::stdx::optional<bool> diacritic_sensitive = bsoncxx::stdx::nullopt) {
    return {search, language, case_sensitive, diacritic_sensitive};
}

/**
 * Creates a query expression with an isolation level set.
 */
template <typename Expr, typename = std::enable_if_t<details::is_query_expression_v<Expr>>>
constexpr isolated_expr<Expr> isolated(const Expr &expr) {
    return {expr};
}

/* Arithmetic update operators */

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_expr<NvpT, typename NvpT::no_opt_type> operator+=(
    const NvpT &nvp, const typename NvpT::no_opt_type &val) {
    return {nvp, val, "$inc"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_value_expr<NvpT, typename NvpT::no_opt_type> operator-=(
    const NvpT &nvp, const typename NvpT::no_opt_type &val) {
    return {nvp, -val, "$inc"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_value_expr<NvpT, typename NvpT::no_opt_type> operator++(const NvpT &nvp) {
    return {nvp, 1, "$inc"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_value_expr<NvpT, typename NvpT::no_opt_type> operator++(const NvpT &nvp, int) {
    return {nvp, 1, "$inc"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_value_expr<NvpT, typename NvpT::no_opt_type> operator--(const NvpT &nvp) {
    return {nvp, -1, "$inc"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_value_expr<NvpT, typename NvpT::no_opt_type> operator--(const NvpT &nvp, int) {
    return {nvp, -1, "$inc"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_arithmetic<typename NvpT::no_opt_type>::value>>
constexpr update_expr<NvpT, typename NvpT::no_opt_type> operator*=(
    const NvpT &nvp, const typename NvpT::no_opt_type &val) {
    return {nvp, val, "$mul"};
}

/* Bitwise update operators */

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_integral<typename NvpT::no_opt_type>::value>>
constexpr bit_update_expr<NvpT, typename NvpT::no_opt_type> operator&=(
    const NvpT &nvp, const typename NvpT::no_opt_type &mask) {
    return {nvp, mask, "and"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_integral<typename NvpT::no_opt_type>::value>>
constexpr bit_update_expr<NvpT, typename NvpT::no_opt_type> operator|=(
    const NvpT &nvp, const typename NvpT::no_opt_type &mask) {
    return {nvp, mask, "or"};
}

template <typename NvpT, typename = std::enable_if_t<is_nvp_v<NvpT>>,
          typename = std::enable_if_t<std::is_integral<typename NvpT::no_opt_type>::value>>
constexpr bit_update_expr<NvpT, typename NvpT::no_opt_type> operator^=(
    const NvpT &nvp, const typename NvpT::no_opt_type &mask) {
    return {nvp, mask, "xor"};
}

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
