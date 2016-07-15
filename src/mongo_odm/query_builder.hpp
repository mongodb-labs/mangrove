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

#include <bsoncxx/builder/core.hpp>
#include <bsoncxx/view_or_value.hpp>

#include <mongo_odm/nvp.hpp>
#include <mongo_odm/util.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

template <typename, typename T>
struct is_bson_appendable {};

// specialization that does the checking
template <typename C, typename Ret, typename... Args>
struct is_bson_appendable<C, Ret(Args...)> {
   private:
    template <typename T>
    static constexpr auto check(T *) ->
        typename std::is_same<decltype(std::declval<T>().append(std::declval<Args>()...)),
                              Ret>::type {
        return {};
    }

    template <typename>
    static constexpr std::false_type check(...) {
        return {};
    }

    typedef decltype(check<C>(0)) type;

   public:
    static constexpr bool value = type::value;
};

/**
 * Templated function for appending a value to a BSON builder. If possible, the function simply
 * passes the value directly to the builder. If it cannot be nicely appended, it is first serialized
 * and then added as a sub-document to the builder. If the value is a container/iterable, it is
 * serialized into a BSON array.
 */
template <typename T>
typename std::enable_if<is_bson_appendable<bsoncxx::builder::core, void(T)>::value, void>::type
append_value_to_bson(T value, bsoncxx::builder::core &builder) {
    builder.append(value);
}

template <typename T>
typename std::enable_if<!is_bson_appendable<bsoncxx::builder::core, void(T)>::value &&
                            !is_iterable<T>::value,
                        void>::type
append_value_to_bson(T value, bsoncxx::builder::core &builder) {
    auto serialized_value = bson_mapper::to_document<T>(value);
    builder.append(bsoncxx::types::b_document{serialized_value});
}

template <typename Iterable>
typename std::enable_if<is_iterable<Iterable>::value, void>::type append_value_to_bson(
    Iterable arr, bsoncxx::builder::core &builder) {
    builder.open_array();
    for (auto x : arr) {
        append_value_to_bson(x, builder);
    }
    builder.close_array();
}

// Forward declarations
template <typename NvpT, typename U>
class NotExpr;

/**
 * Represents a query expression with the syntax "key: {$op: value}".
 * This usually means queries that are comparisons, such as (User.age > 21).
 * However, this also covers operators such as $exists.
 */
template <typename NvpT, typename U>
class ComparisonExpr {
   public:
    /**
     * Constructs a query expression for the given key, value, and comparison type
     * @param  nvp           A name-value pair corresponding to a key in a document
     * @param  field         The value that the key is being compared to.
     * @param  selector_type The type of comparison operator, such at gt (>) or ne (!=).
     */
    constexpr ComparisonExpr(const NvpT &nvp, const U &field, const char *selector_type)
        : _nvp(nvp), _field(field), _selector_type(selector_type) {
    }
    /**
     * Appends this expression to a BSON core builder, as a key-value pair of the form
     * "key: {$cmp: val}", where $cmp is some comparison operator.
     * @param builder A BSON core builder
     * @param wrap  Whether to wrap the BSON inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_nvp.get_name());
        builder.open_document();
        builder.key_view(_selector_type);
        // NOTE: currently this can only append types that the builder can capture.
        // (Hmmm.... if only we had some way of easily converting C++ objects to BSON.... )
        // TODO: Look into using the BSON archiver here for _field.
        append_value_to_bson(_field, builder);
        // builder.append(_field);
        builder.close_document();
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

    friend NotExpr<NvpT, U>;

   private:
    const NvpT _nvp;
    const U &_field;
    const char *_selector_type;
};

/**
 * This represents an expression with the $not operator, which wraps a comparison expression and
 * negates it.
 */
template <typename NvpT, typename U>
class NotExpr {
   public:
    /**
     * Creates a $not expression that negates the given comparison expression.
     * @param  expr A comparison expression
     */
    constexpr NotExpr(const ComparisonExpr<NvpT, U> &expr) : _expr(expr) {
    }

    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form "key: {$not: {$cmp: val}}".
     * @param builder a BSON core builder
     * @param wrap  Whether to wrap the BSON in a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_expr._nvp.get_name());
        builder.open_document();
        builder.key_view("$not");
        builder.open_document();
        builder.key_view(_expr._selector_type);
        append_value_to_bson(_expr._field, builder);
        // builder.append(_expr._field);
        builder.close_document();
        builder.close_document();
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
    const ComparisonExpr<NvpT, U> _expr;
};

/**
 * An expression that compares a key-value pair to a set of values in an iterable.
 * @tparam NvpT     The type of the key-value pair used in this expression.
 * @tparam Iterable A type that can be used inside a range-based for loop. The objects retreived by
 * iterating must be convertible to the type of the key-value pair.
 */
template <typename NvpT, typename Iterable>
class InArrayExpr {
   public:
    constexpr InArrayExpr(const NvpT &nvp, const Iterable &iter, const bool negate = false)
        : _nvp(nvp), _iter(iter), _negate(negate) {
    }

    /**
     * Appends this expression to a BSON core builder, as a key-value pair of the form
     * "key: {$cmp: val}", where $cmp is some comparison operator.
     * @param builder A BSON core builder
     * @param wrap  Whether to wrap the BSON inside a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_nvp.get_name());
        builder.open_document();
        builder.key_view(_negate ? "$nin" : "$in");
        append_value_to_bson(_iter, builder);
        builder.close_document();
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
    const NvpT _nvp;
    const Iterable &_iter;
    const bool _negate;
};

/**
 * This class represents a query expression using the $mod operator, that checks the modulus of a
 * certain numerical field.
 */
template <typename NvpT>
class ModExpr {
   public:
    /**
     * Constructs a ModExpr that represents a query with the $mod operator.
     * @param  nvp       The name-value pair to compare against.
     * @param  divisor   The divisor used in the modulus operation.
     * @param  remainder The remainder produced when dividing the field by the divisor.
     */
    constexpr ModExpr(NvpT nvp, const int &divisor, const int &remainder)
        : _nvp(nvp), _divisor(divisor), _remainder(remainder) {
    }

    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form " key: { $mod: [ divisor, remainder ] } "
.
     * @param builder a BSON core builder
     * @param wrap  Whether to wrap the BSON in a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_nvp.get_name());
        builder.open_document();
        builder.key_view("$mod");
        builder.open_array();
        builder.append(_divisor);
        builder.append(_remainder);
        builder.close_array();
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
    const NvpT _nvp;
    const int &_divisor;
    const int &_remainder;
};

/**
 * Represents a query that performs a text search with the $text operator.
 * TODO check values of `language` against spec?
 */
class TextSearchExpr {
   public:
    /**
     * Creates a text search expression.
     * These parameters correspond to the parameters for the $text operator in MongoDB.
     * @param  search               A string of terms use to query the text index.
     * @param  language             The language of the text index.
     * @param  case_sensitive      A boolean flag to specify case-sensitive search. Optional, false
     *                             by default.
     * @param  diacritic_sensitive A boolean flag to specify case-sensitive search. Optional, false
     *                             by default.
     */
    constexpr TextSearchExpr(const char *search, const char *language, bool case_sensitive = false,
                             bool diacritic_sensitive = false)
        : _search(search),
          _language(language),
          _case_sensitive(case_sensitive),
          _diacritic_sensitive(diacritic_sensitive) {
    }

    /**
     * Creates a text search expression, with the default `language` setting of the text index.
     * These parameters correspond to the parameters for the $text operator in MongoDB.
     * @param  search               A string of terms use to query the text index.
     * @param  case_sensitive      A boolean flag to specify case-sensitive search. Optional, false
     *                             by default.
     * @param  diacritic_sensitive A boolean flag to specify case-sensitive search. Optional, false
     *                             by default.
     */
    constexpr TextSearchExpr(const char *search, bool case_sensitive = false,
                             bool diacritic_sensitive = false)
        : _search(search),
          _case_sensitive(case_sensitive),
          _diacritic_sensitive(diacritic_sensitive) {
    }

    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form " key: { $mod: [ divisor, remainder ] } "
    .
     * @param builder a BSON core builder
     * @param wrap  Whether to wrap the BSON in a document.
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
        builder.key_view("$caseSensitive");
        builder.append(_case_sensitive);
        builder.key_view("$diacriticSensitive");
        builder.append(_diacritic_sensitive);
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
    const mongocxx::stdx::optional<const char *> _language;
    const bool _case_sensitive;
    const bool _diacritic_sensitive;
};

template <typename NvpT>
class RegexExpr {
   public:
    constexpr RegexExpr(NvpT nvp, const char *regex, const char *options = "    ")
        : _nvp(nvp), _regex(regex), _options(options) {
    }
    /**
     * Appends this expression to a BSON core builder,
     * as a key-value pair of the form " key: { $mod: [ divisor, remainder ] } "
    .
     * @param builder a BSON core builder
     * @param wrap  Whether to wrap the BSON in a document.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        if (wrap) {
            builder.open_document();
        }
        builder.key_view(_nvp.get_name());
        builder.open_document();
        builder.key_view("$regex");
        builder.append(_regex);
        builder.key_view("$options");
        builder.append(_options);
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
    const NvpT _nvp;
    const char *_regex;
    const char *_options;
};

template <typename Head, typename Tail>
class ExpressionList {
   public:
    /**
     * Constructs an expression list.
     * @param head  The first element in the list.
     * @param tail  The remainder of the list
     */
    constexpr ExpressionList(const Head &head, const Tail &tail) : _head(head), _tail(tail) {
    }

    /**
     * Appends this expression list to the given core builder by appending the first expression
     * in the list, and then recursing on the rest of the list.
     * @param builder A basic BSON core builder.
     * @param Whether to wrap individual elements inside a document. This is useful when the
     * ExpressionList is used as a BSON array.
     */
    void append_to_bson(bsoncxx::builder::core &builder, bool wrap = false) const {
        _head.append_to_bson(builder, wrap);
        _tail.append_to_bson(builder, wrap);
    }

    /**
     * Casts the expression list to a BSON query of  the form { expr1, expr2, ....}
     */
    operator bsoncxx::document::view_or_value() const {
        auto builder = bsoncxx::builder::core(false);
        append_to_bson(builder);
        return builder.extract_document();
    }

   private:
    const Head _head;
    const Tail _tail;
};

/**
 * Function for creating an ExpressionList out of a variadic list of epxressions.
 */
template <typename Expr1, typename Expr2, typename... Expressions>
constexpr auto make_expression_list(Expr1 e1, Expr2 e2, Expressions... args) {
    // Expr1 is the "tail" that contains the intermediate expression list, Expr2 is a single
    // expression being added onto the list.
    return make_expression_list(ExpressionList<Expr2, Expr1>(e2, e1), args...);
}

/**
 * Base case for variadic ExpressionList builder, simply returns the list it receives.
 */
template <typename List>
constexpr List make_expression_list(List l) {
    return l;
}

/**
 * This represents a boolean expression with two arguments.
 */
template <typename Expr1, typename Expr2>
class BooleanExpr {
   public:
    /**
     * Constructs a boolean expression from two other expressions, and a certain operator.
     * @param  lhs The left-hand side of the expression.
     * @param  rhs The right-hand side of the expression.
     * @param  op  The operator of the expression, e.g. AND or OR.
     */
    constexpr BooleanExpr(const Expr1 &lhs, const Expr2 &rhs, const char *op)
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
class BooleanListExpr {
   public:
    /**
     * Constructs a boolean expression from a list of sub-expressions, and a certain operator.
     * @param args An expression list of boolean conditions.
     * @param  op  The operator of the expression, e.g. AND or OR.
     */
    constexpr BooleanListExpr(const List args, const char *op) : _args(args), _op(op) {
    }

    /**
     * Appends this query to a BSON core builder as a key-value pair "$op: [{lhs}, {rhs}]"
     * @param builder A basic BSON core builder.
     * @param wrap  Whether to wrap this expression inside a document.
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
 * Represents an update operator that modifies a certain elements.
 * This creates BSON expressions of the form "$op: {field: value}",
 * where $op can be $set, $inc, etc.
 *
 * The value should be convertible to the type of the name-value pair. (For an optional field, the
 * type wrapped by the stdx::optional.)
 */
template <typename NvpT>
class UpdateExpr {
   public:
    constexpr UpdateExpr(const NvpT &nvp, const typename NvpT::no_opt_type &val, const char *op)
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
        builder.key_view(_nvp.get_name());
        builder.append(_val);
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
    NvpT _nvp;
    typename NvpT::no_opt_type _val;
    const char *_op;
};

/* A templated struct that holds a boolean value.
* This value is TRUE for types that are query expressions,
* and FALSE for all other types.
*/
template <typename>
struct is_query_expression : public std::false_type {};

template <typename NvpT, typename U>
struct is_query_expression<ComparisonExpr<NvpT, U>> : public std::true_type {};

template <typename NvpT, typename U>
struct is_query_expression<NotExpr<NvpT, U>> : public std::true_type {};

template <typename NvpT, typename Iterable>
struct is_query_expression<InArrayExpr<NvpT, Iterable>> : public std::true_type {};

template <typename NvpT>
struct is_query_expression<ModExpr<NvpT>> : public std::true_type {};

template <>
struct is_query_expression<TextSearchExpr> : public std::true_type {};

template <typename NvpT>
struct is_query_expression<RegexExpr<NvpT>> : public std::true_type {};

template <typename Head, typename Tail>
struct is_query_expression<ExpressionList<Head, Tail>> {
    constexpr static bool value =
        is_query_expression<Head>::value && is_query_expression<Tail>::value;
};

template <typename Expr1, typename Expr2>
struct is_query_expression<BooleanExpr<Expr1, Expr2>> : public std::true_type {};

template <typename List>
struct is_query_expression<BooleanListExpr<List>> : public std::true_type {};

/**
 * A templated struct that holds a boolean value.
 * This value is TRUE for types that are update expressions, and false otherwise.
 */
template <typename>
struct is_update_expression : public std::false_type {};

template <typename NvpT>
struct is_update_expression<UpdateExpr<NvpT>> : public std::true_type {};

template <typename Head, typename Tail>
struct is_update_expression<ExpressionList<Head, Tail>> {
    constexpr static bool value =
        is_update_expression<Head>::value && is_update_expression<Tail>::value;
};

/* Operator overloads for creating and combining expressions */

/* Overload comparison operators for name-value pairs to create expressions.
 */

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> eq(const NvpT &lhs, const U &rhs) {
    return {lhs, rhs, "$eq"};
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> operator==(const NvpT &lhs, const U &rhs) {
    return eq(lhs, rhs);
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> gt(const NvpT &lhs, const U &rhs) {
    return {lhs, rhs, "$gt"};
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> operator>(const NvpT &lhs, const U &rhs) {
    return gt(lhs, rhs);
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> gte(const NvpT &lhs, const U &rhs) {
    return {lhs, rhs, "$gte"};
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> operator>=(const NvpT &lhs, const U &rhs) {
    return gte(lhs, rhs);
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> lt(const NvpT &lhs, const U &rhs) {
    return {lhs, rhs, "$lt"};
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> operator<(const NvpT &lhs, const U &rhs) {
    return lt(lhs, rhs);
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> lte(const NvpT &lhs, const U &rhs) {
    return {lhs, rhs, "$lte"};
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> operator<=(const NvpT &lhs, const U &rhs) {
    return lte(lhs, rhs);
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> ne(const NvpT &lhs, const U &rhs) {
    return {lhs, rhs, "$ne"};
}

template <
    typename NvpT, typename U, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_convertible<U, typename NvpT::type>::value>::type>
constexpr ComparisonExpr<NvpT, U> operator!=(const NvpT &lhs, const U &rhs) {
    return ne(lhs, rhs);
}

/**
 * Negates a comparison expression in a $not expression.
 */
template <typename NvpT, typename U,
          typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type>
constexpr NotExpr<NvpT, U> operator!(const ComparisonExpr<NvpT, U> &expr) {
    return {expr};
}

/**
 * Comma operator that combines two expressions or an expression and an expression list into a
 * new expression list.
 * NOTE: This recursively checks that the elements of the list are either all query expressions or
 * all update expressions. This happens each time an element is appended to the list, so this could
 * get expensive. Perhaps we could store the "expression category" inside the ExpressionList object?
 */
template <typename Head, typename Tail,
          typename = typename std::enable_if<
              (is_query_expression<Head>::value && is_query_expression<Tail>::value) ||
              (is_update_expression<Head>::value && is_update_expression<Tail>::value)>::type>
constexpr ExpressionList<Head, Tail> operator,(Tail &&tail, Head &&head) {
    return {head, tail};
}

/**
 * Boolean operator overloads for expressions.
 */
template <typename Expr1, typename Expr2,
          typename = typename std::enable_if<is_query_expression<Expr1>::value &&
                                             is_query_expression<Expr2>::value>::type>
constexpr BooleanExpr<Expr1, Expr2> operator&&(const Expr1 &lhs, const Expr2 &rhs) {
    return {lhs, rhs, "$and"};
}

template <typename Expr1, typename Expr2,
          typename = typename std::enable_if<is_query_expression<Expr1>::value &&
                                             is_query_expression<Expr2>::value>::type>
constexpr BooleanExpr<Expr1, Expr2> operator||(const Expr1 &lhs, const Expr2 &rhs) {
    return {lhs, rhs, "$or"};
}

// TODO I'd like list ops for "and" and "or" as well, but these are reserved keywords.
// Perhaps use list_and, list_or, list_nor instead?
// Currently chaining &&'s or ||'s provides the same results as a boolean operation on a list, and
// not much different in terms of performance.

/**
 * A function that creates a $nor operator out of an ExpressionList containing arguments
 * @param list The list of arguments to the $nor operator, as an expression list.
 * @return A BooleanLlistExpr that wraps the given list.
 */
template <typename Head, typename Tail,
          typename =
              typename std::enable_if<is_query_expression<ExpressionList<Head, Tail>>::value>::type>
constexpr BooleanListExpr<ExpressionList<Head, Tail>> nor(const ExpressionList<Head, Tail> &list) {
    return {list, "$nor"};
}

/**
 * Function that creates a $nor operator out of a list of arguments.
 */
template <typename... QueryExpressions,
          typename = typename all_true<is_query_expression<QueryExpressions>::value...>::type>
constexpr auto nor(QueryExpressions... args)
    -> BooleanListExpr<decltype(make_expression_list(args...))> {
    return {make_expression_list(args...), "$nor"};
}

/**
* Creates a text search expression.
* These parameters correspond to the parameters for the $text operator in MongoDB.
* @param  search               A string of terms use to query the text index.
* @param  language             The language of the text index.
* @param  case_sensitive      A boolean flag to specify case-sensitive search. Optional, false
*                             by default.
* @param  diacritic_sensitive A boolean flag to specify case-sensitive search. Optional, false
*                             by default.
* @return                     A TextSearchExpr with the given parameters.
*/
constexpr TextSearchExpr text(const char *search, const char *language, bool case_sensitive = false,
                              bool diacritic_sensitive = false) {
    return {search, language, case_sensitive, diacritic_sensitive};
}

/**
 * Creats a TextSearchExpr, with the $language parameter unspecified.
 * See documentation for `TextSearchExpr text(const char *search, const char *language, bool
                                              case_sensitive, bool diacritic_sensitive)` above.
 */
constexpr TextSearchExpr text(const char *search, bool case_sensitive = false,
                              bool diacritic_sensitive = false) {
    return {search, case_sensitive, diacritic_sensitive};
}

/**
 * Arithmetic update operators.
 * TODO: allow stdx::optional arithmetic types to work with this as well.
 */

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator+=(const NvpT &nvp, const typename NvpT::no_opt_type &val) {
    return {nvp, val, "$inc"};
}

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator-=(const NvpT &nvp, const typename NvpT::no_opt_type &val) {
    return {nvp, -val, "$inc"};
}

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator++(const NvpT &nvp) {
    return {nvp, 1, "$inc"};
}

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator++(const NvpT &nvp, int) {
    return {nvp, 1, "$inc"};
}

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator--(const NvpT &nvp) {
    return {nvp, -1, "$inc"};
}

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator--(const NvpT &nvp, int) {
    return {nvp, -1, "$inc"};
}

template <
    typename NvpT, typename = typename std::enable_if<is_nvp_type<NvpT>::value>::type,
    typename = typename std::enable_if<std::is_arithmetic<typename NvpT::no_opt_type>::value>::type>
constexpr UpdateExpr<NvpT> operator*=(const NvpT &nvp, const typename NvpT::no_opt_type &val) {
    return {nvp, val, "$mul"};
}

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
