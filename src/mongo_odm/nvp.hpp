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
    typename std::enable_if<(I == N), void>::type mongo_odm_serialize_recur(Archive&) {         \
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

template <typename Base, typename T, typename Parent>
class NvpChild {
   public:
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
template <typename Base, typename T, size_t N, size_t M, T Base::*t>
    constexpr std::enable_if_t < N<M && !hasField<Base, T, N, M>::value, bool> wrapbool();

template <typename Base, typename T, size_t N, size_t M, T Base::*>
constexpr std::enable_if_t<N == M, bool> wrapbool();

template <typename Base, typename T, size_t N, size_t M, T Base::*t>
    constexpr std::enable_if_t < N<M && hasField<Base, T, N, M>::value, bool> wrapbool() {
    if (std::get<N>(Base::mongo_odm_mapped_fields()).t == t) {
        return true;
    } else {
        return wrapbool<Base, T, N + 1, M, t>();
    }
}

template <typename Base, typename T, size_t N, size_t M, T Base::*t>
    constexpr std::enable_if_t < N<M && !hasField<Base, T, N, M>::value, bool> wrapbool() {
    return wrapbool<Base, T, N + 1, M, t>();
}

template <typename Base, typename T, size_t N, size_t M, T Base::*>
constexpr std::enable_if_t<N == M, bool> wrapbool() {
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
constexpr std::enable_if_t<N == M, const Nvp<Base, T>> wrapimpl(T Base::*) {
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
        Base, T, 0, std::tuple_size<decltype(Base::mongo_odm_mapped_fields())>::value, ptr>()>> {
    static constexpr const Nvp<Base, T> call() {
        return wrap(ptr);
    }
};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
