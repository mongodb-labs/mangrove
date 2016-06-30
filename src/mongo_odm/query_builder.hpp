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

#include <mongo_odm/config/prelude.hpp>

#include <cstddef>
#include <stdexcept>
#include <tuple>
#include <type_traits>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/view_or_value.hpp>

// TODO consider how namespaces will work with these macros?
#define NVP(x) makeNvp(&wrapBase::x, #x)

#define ADAPT(Base, ...)   \
    using wrapBase = Base; \
    constexpr static auto fields = std::make_tuple(__VA_ARGS__);

#define ADAPT_STORAGE(Base) constexpr decltype(Base::fields) Base::fields

#define SAFEWRAP(name, member) \
    hasCallIfFieldIsPresent<decltype(&name::member), &name::member>::call()

#define SAFEWRAPTYPE(value) hasCallIfFieldIsPresent<decltype(&value), &value>::call()

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

/**
 * An object that represents a name-value pair of a member in an object.
 * It is templated on the class of the member and its type.
 */
template <typename Base, typename T>
struct Nvp {
    constexpr Nvp(T Base::*t, const char *name) : t(t), name(name) {
    }

    T Base::*t;
    const char *name;
};

// Enumeration of possible query selectors, as well as a mapping to string values
enum QuerySelector { eq, gt, gte, lt, lte, ne, in, nin };
const std::map<QuerySelector, std::string> selector_map = {
    {eq, "$eq"},   {gt, "$gt"}, {gte, "$gte"}, {lt, "$lt"},
    {lte, "$lte"}, {ne, "$ne"}, {in, "$in"},   {nin, "$nin"}};

/**
 * Represents a query expression involving name-value pairs.
 */
template <typename Base, typename T>
class Expr {
   public:
    constexpr Expr(const Nvp<Base, T> &nvp, T field, QuerySelector selector_type)
        : nvp(nvp),
          field(std::move(field)),
          selector_type(selector_map.find(selector_type)->second) {
    }

    const Nvp<Base, T> &nvp;
    T field;
    const std::string selector_type;

    /**
     * Converts the expression to a BSON filter for a query.
     */
    operator bsoncxx::document::view_or_value() {
        auto builder = bsoncxx::builder::stream::document{};
        auto value = builder << std::string(nvp.name) << bsoncxx::builder::stream::open_document
                             << selector_type << field << bsoncxx::builder::stream::close_document
                             << bsoncxx::builder::stream::finalize;
        return bsoncxx::document::view_or_value(value);
    }

    friend std::ostream &operator<<(std::ostream &os, const Expr &expr) {
        os << "{ " << expr.nvp.name << ": {" << expr.selector_type << ": " << expr.field << "} }";
        return os;
    }
};

/* Overload operators for name-value pairs to create expressions */
// Separate operators are defined for bool types to prevent unnecesary implicit casting to bool for
// non-bool types.
template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator==(const Nvp<Base, T> &lhs, const U &rhs) {
    return Expr<Base, T>(lhs, rhs, eq);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator==(const Nvp<Base, T> &lhs, const T &rhs) {
    return Expr<Base, T>(lhs, rhs, eq);
}

template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator>(const Nvp<Base, T> &lhs, const U &rhs) {
    return Expr<Base, T>(lhs, rhs, gt);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator>(const Nvp<Base, T> &lhs, const T &rhs) {
    return Expr<Base, T>(lhs, rhs, gt);
}

template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator>=(const Nvp<Base, T> &lhs, const U &rhs) {
    return Expr<Base, T>(lhs, rhs, gte);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator>=(const Nvp<Base, T> &lhs, const T &rhs) {
    return Expr<Base, T>(lhs, rhs, gte);
}

template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator<(const Nvp<Base, T> &lhs, const U &rhs) {
    return Expr<Base, T>(lhs, rhs, lt);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator<(const Nvp<Base, T> &lhs, const T &rhs) {
    return Expr<Base, T>(lhs, rhs, lt);
}

template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator<=(const Nvp<Base, T> &lhs, const U &rhs) {
    return Expr<Base, T>(lhs, rhs, lte);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator<=(const Nvp<Base, T> &lhs, const T &rhs) {
    return Expr<Base, T>(lhs, rhs, lte);
}

template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator!=(const Nvp<Base, T> &lhs, const U &rhs) {
    return Expr<Base, T>(lhs, rhs, ne);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator!=(const Nvp<Base, T> &lhs, const T &rhs) {
    return Expr<Base, T>(lhs, rhs, ne);
}

// Create a name-value pair from a object member and its name
template <typename Base, typename T>
Nvp<Base, T> constexpr makeNvp(T Base::*t, const char *name) {
    return Nvp<Base, T>(t, name);
}

/**
 * hasField determines whether a type Base has a member of the given type T as
 * the Nth member out of M total members which have name value pairs.
 */
// By default, if N>=M the index is out of bounds and hasField is false-y.
template <typename Base, typename T, size_t N, size_t M,
          bool = N<M> struct hasField : public std::false_type {};

// Nth member in the Base::fields tuple (i.e. the list of fields for which we
// have name-value pairs)
// Must have same type as the given argument.
template <typename Base, typename T, size_t N, size_t M>
struct hasField<Base, T, N, M, true>
    : public std::is_same<T Base::*, decltype(std::get<N>(Base::fields).t)> {};

// forward declarations for wrapimpl
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T> *> wrapimpl(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && !hasField<Base, T, N, M>::value, const Nvp<Base, T> *> wrapimpl(T Base::*t);

/**
 * wrapimpl uses template arguments N and M to iterate over the fields of a Base
 * class, and returns the name-value pair corresponding to the given member
 * field.
 */
template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && hasField<Base, T, N, M>::value, const Nvp<Base, T> *> wrapimpl(T Base::*t) {
    if (std::get<N>(Base::fields).t == t) {
        return &std::get<N>(Base::fields);
    } else {
        return wrapimpl<Base, T, N + 1, M>(t);
    }
}

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && !hasField<Base, T, N, M>::value, const Nvp<Base, T> *> wrapimpl(T Base::*t) {
    return wrapimpl<Base, T, N + 1, M>(t);
}

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T> *> wrapimpl(T Base::*t) {
    return nullptr;
}

/**
 * Returns a name-value pair corresponding to the given member pointer.
 */
template <typename Base, typename T>
constexpr const Nvp<Base, T> *wrap(T Base::*t) {
    return wrapimpl<Base, T, 0, std::tuple_size<decltype(Base::fields)>::value>(t);
}

/*
A struct that has a call() method that returns a name-value pair corresponding
to the given member pointer,
but only if such a member exists.
 */
template <typename T, T, typename = void>
struct hasCallIfFieldIsPresent {};

template <typename Base, typename T, T Base::*ptr>
struct hasCallIfFieldIsPresent<T Base::*, ptr, std::enable_if_t<wrap(ptr) != nullptr>> {
    static constexpr const Nvp<Base, T> &call() {
        return *wrap(ptr);
    }
};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
