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

#define PP_NARG(...) PP_NARG_(__VA_ARGS__, PP_RSEQ_N())
#define PP_NARG_(...) PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, \
                 _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34,  \
                 _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,  \
                 _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...)         \
    N
#define PP_RSEQ_N()                                                                             \
    63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, \
        40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, \
        18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define MONGO_ODM_NVP(x) mongo_odm::makeNvp(&mongo_odm_wrap_base::x, #x)

// Creates serialize() function
#define MONGO_ODM_SERIALIZE_KEYS                                                                \
    template <class Archive>                                                                    \
    void serialize(Archive& ar) {                                                               \
        mongo_odm_serialize_recur<Archive, 0,                                                   \
                                  std::tuple_size<decltype(mongo_odm_mapped_fields())>::value>( \
            ar);                                                                                \
    }                                                                                           \
    template <class Archive, size_t I, size_t N>                                                \
    typename std::enable_if<(I < N), void>::type mongo_odm_serialize_recur(Archive& ar) {       \
        auto nvp = std::get<I>(mongo_odm_mapped_fields());                                      \
        ar(cereal::make_nvp(nvp.name, this->*(nvp.t)));                                         \
        mongo_odm_serialize_recur<Archive, I + 1, N>(ar);                                       \
    }                                                                                           \
                                                                                                \
    template <class Archive, size_t I, size_t N>                                                \
    typename std::enable_if<(I == N), void>::type mongo_odm_serialize_recur(Archive& ar) {      \
        ;                                                                                       \
    }

// Register members and create serialize() function
#define MONGO_ODM_MAKE_KEYS(Base, ...)                \
    using mongo_odm_wrap_base = Base;                 \
    constexpr static auto mongo_odm_mapped_fields() { \
        return std::make_tuple(__VA_ARGS__);          \
    }                                                 \
    MONGO_ODM_SERIALIZE_KEYS

// If using the mongo_odm::model, then also register _id as a field.
#define MONGO_ODM_MAKE_KEYS_MODEL(Base, ...) \
    MONGO_ODM_MAKE_KEYS(Base, MONGO_ODM_NVP(_id), __VA_ARGS__)

#define MONGO_ODM_KEY(value) mongo_odm::hasCallIfFieldIsPresent<decltype(&value), &value>::call()
#define MONGO_ODM_KEY_BY_VALUE(value) hasCallIfFieldIsPresent<decltype(value), value>::call()

#define MONGO_ODM_CHILD1(base, field1) ODM_KEY_BY_VALUE(&base::field1)

#define MONGO_ODM_CHILD2(base, field1, field2)                                               \
    makeNvpWithParent(                                                                       \
        MONGO_ODM_KEY_BY_VALUE(                                                              \
            &std::decay_t<typename decltype(MONGO_ODM_CHILD1(base, field1))::type>::field2), \
        MONGO_ODM_CHILD1(base, field1))

#define MONGO_ODM_CHILD3(base, field1, field2, field3)                                         \
    makeNvpWithParent(MONGO_ODM_KEY_BY_VALUE(&std::decay_t<typename decltype(MONGO_ODM_CHILD2( \
                                                 base, field1, field2))::type>::field3),       \
                      MONGO_ODM_CHILD2(base, field1, field2))

#define PASTE_IMPL(s1, s2) s1##s2
#define PASTE(s1, s2) PASTE_IMPL(s1, s2)

#define MONGO_ODM_CHILD(type, ...) PASTE(MONGO_ODM_CHILD, PP_NARG(__VA_ARGS__))(type, __VA_ARGS__)

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

template <typename Base, typename T, typename Parent>
class NvpChild;

template <typename T, typename Parent>
class UpdateExpr;

/**
 * An object that represents a name-value pair of a member in an object.
 * It is templated on the class of the member and its type.
 */
template <typename Base, typename T>
struct Nvp {
    using type = T;

    /**
     * Create a name-value pair from a member pointer and a name.
     * @param  t    A pointer to the member
     * @param  name The name of the member
     */
    constexpr Nvp(T Base::*t, const char* name) : t(t), name(name) {
    }

    template <typename U>
    constexpr NvpChild<T, U, Nvp<Base, T>> operator->*(const Nvp<T, U>& child) const {
        return {child.t, child.name, *this};
    }

    constexpr UpdateExpr<T, Nvp<Base, T>> operator=(const T& val) const {
        return {*this, val, "$set"};
    }

    std::string get_name() const {
        return name;
    }

    T Base::*t;
    const char* name;
};

/**
 * Class that represents a name-value pair for a field of an object that is a member of another
 * object.
 * The name of this name-value pair will be "<parent-name>.field_name".
 * The parenet can be either an Nvp, or another NvpChild.
 * @tparam Base The type of the object that contains this field
 * @tparam T    The type of the field
 * @tparam Parent The type of the parent name-value pair, either an Nvp<...> or NvpChild<...>.
 */
template <typename Base, typename T, typename Parent>
class NvpChild {
   public:
    using type = T;

    constexpr NvpChild(T Base::*t, const char* name, Parent parent)
        : t(t), name(name), parent(parent) {
    }

    template <typename U>
    constexpr NvpChild<T, U, NvpChild<Base, T, Parent>> operator->*(const Nvp<T, U>& child) const {
        return {child.t, child.name, *this};
    }

    constexpr UpdateExpr<T, NvpChild<Base, T, Parent>> operator=(const T& val) const {
        return {*this, val, "$set"};
    }

    std::string get_name() const {
        std::string full_name;
        full_name += (parent.get_name() + ".");
        full_name += name;
        return full_name;
    }

    const char* name;
    T Base::*t;
    const Parent parent;
};

/* Create a name-value pair from a object member and its name */
template <typename Base, typename T>
Nvp<Base, T> constexpr makeNvp(T Base::*t, const char* name) {
    return Nvp<Base, T>(t, name);
}

/**
 * Constructs a name-value pair that is a subfield of a `parent` object.
 * The resulting name-value pair will have the name "rootfield.subfield".
 */
template <typename Base, typename T, typename Parent>
NvpChild<Base, T, Parent> constexpr makeNvpWithParent(const Nvp<Base, T>& child,
                                                      const Parent& parent) {
    return NvpChild<Base, T, Parent>(child.t, child.name, parent);
}
/**
 * hasField determines whether a type Base has a member of the given type T as
 * the Nth member out of M total members which have name value pairs.
 */
// By default, if N>=M the index is out of bounds and hasField is false-y.
template <typename Base, typename T, size_t N, size_t M, bool = (N < M)>
struct hasField : public std::false_type {};

// Nth member in the Base::fields tuple (i.e. the list of fields for which we
// have name-value pairs)
// Must have same type as the given argument.
template <typename Base, typename T, size_t N, size_t M>
struct hasField<Base, T, N, M, true>
    : public std::is_same<T Base::*, decltype(std::get<N>(Base::mongo_odm_mapped_fields()).t)> {};

/**
 * wrapbool iterates through the fields and returns true if a matching member exists, and false
 * otherwise.
 */

// forward declarations for wrapbool
template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t < N<M && !hasField<Base, T, N, M>::value, bool> wrapbool(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, bool> wrapbool(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t < N<M && hasField<Base, T, N, M>::value, bool> wrapbool(T Base::*t) {
    if (std::get<N>(Base::mongo_odm_mapped_fields()).t == t) {
        return true;
    } else {
        return wrapbool<Base, T, N + 1, M>(t);
    }
}

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && !hasField<Base, T, N, M>::value, bool> wrapbool(T Base::*t) {
    return wrapbool<Base, T, N + 1, M>(t);
}

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, bool> wrapbool(T Base::*t) {
    return false;
}

/**
 * wrapimpl uses template arguments N and M to iterate over the fields of a Base
 * class, and returns the name-value pair corresponding to the given member
 * field.
 */

// forward declarations for wrapimpl
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T>> wrapimpl(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && !hasField<Base, T, N, M>::value, const Nvp<Base, T>> wrapimpl(
    T Base::*t);

// If Nth field has same type as T, check that it points to the same member.
// If not, check (N+1)th field.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && hasField<Base, T, N, M>::value, const Nvp<Base, T>> wrapimpl(
    T Base::*t) {
    if (std::get<N>(Base::mongo_odm_mapped_fields()).t == t) {
        return std::get<N>(Base::mongo_odm_mapped_fields());
    } else {
        return wrapimpl<Base, T, N + 1, M>(t);
    }
}

// If current field doesn't match the type of T, check (N+1)th field.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<(N < M) && !hasField<Base, T, N, M>::value, const Nvp<Base, T>> wrapimpl(
    T Base::*t) {
    return wrapimpl<Base, T, N + 1, M>(t);
}

// If N==M, we've gone past the last field, return nullptr.
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T>> wrapimpl(T Base::*t) {
    return Nvp<Base, T>(nullptr, nullptr);
}

/**
 * Returns a name-value pair corresponding to the given member pointer.
 * @tparam Base The class containing the member in question
 * @tparam T    The type of the member
 * @param t     A member pointer to an element of type T in the class Base.
 * @return      The name-value pair corresponding to this member pointer.
 */
template <typename Base, typename T>
constexpr const Nvp<Base, T> wrap(T Base::*t) {
    return wrapimpl<Base, T, 0, std::tuple_size<decltype(Base::mongo_odm_mapped_fields())>::value>(
        t);
}

/**
 * A struct that has a call() method that returns a name-value pair corresponding
 * to the given member pointer,
 * but only if such a member exists.
 */
template <typename T, T, typename = void>
struct hasCallIfFieldIsPresent {};

template <typename Base, typename T, T Base::*ptr>
struct hasCallIfFieldIsPresent<
    T Base::*, ptr,
    std::enable_if_t<wrapbool<
        Base, T, 0, std::tuple_size<decltype(Base::mongo_odm_mapped_fields())>::value>(ptr)>> {
    static constexpr const Nvp<Base, T> call() {
        return wrap(ptr);
    }
};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
