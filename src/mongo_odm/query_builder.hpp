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

#define NVP(x) makeNvp(&wrapBase::x, #x)

#define ADAPT(Base, ...)                                                       \
  using wrapBase = Base;                                                       \
  constexpr static auto fields = std::make_tuple(__VA_ARGS__);

#define ADAPT_STORAGE(Base) constexpr decltype(Base::fields) Base::fields

#define SAFEWRAP(name, member)                                                 \
  hasCallIfFieldIsPresent<decltype(&name::member), &name::member>::call()

#define SAFEWRAPTYPE(value)                                                    \
  hasCallIfFieldIsPresent<decltype(&value), &value>::call()

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

template <typename Base, typename T> struct Nvp {
  constexpr Nvp(T Base::*t, const char *name) : t(t), name(name) {}

  T Base::*t;
  const char *name;
};

template <typename Base, typename T> struct Expr {
  constexpr Expr(const Nvp<Base, T> &nvp, T field)
      : nvp(nvp), field(std::move(field)) {}

  const Nvp<Base, T> &nvp;
  T field;

  friend std::ostream &operator<<(std::ostream &os, const Expr &expr) {
    os << expr.nvp.name << " == " << expr.field;
    return os;
  }
};

template <typename Base, typename T, typename U,
          typename = typename std::enable_if_t<!std::is_same<T, bool>::value>>
Expr<Base, T> operator==(const Nvp<Base, T> &lhs, const U &rhs) {
  return Expr<Base, T>(lhs, rhs);
}

template <typename Base, typename T,
          typename = typename std::enable_if_t<std::is_same<T, bool>::value>>
Expr<Base, T> operator==(const Nvp<Base, T> &lhs, const T &rhs) {
  return Expr<Base, T>(lhs, rhs);
}

template <typename Base, typename T>
Nvp<Base, T> constexpr makeNvp(T Base::*t, const char *name) {
  return Nvp<Base, T>(t, name);
}

template <typename Base, typename T, size_t N, size_t M,
          bool = N<M> struct hasField : public std::false_type {};

template <typename Base, typename T, size_t N, size_t M>
struct hasField<Base, T, N, M, true>
    : public std::is_same<T Base::*, decltype(std::get<N>(Base::fields).t)> {};

// forward declarations for wrapimpl
template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T> *> wrapimpl(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && !hasField<Base, T, N, M>::value, const Nvp<Base, T> *>
    wrapimpl(T Base::*t);

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && hasField<Base, T, N, M>::value, const Nvp<Base, T> *>
    wrapimpl(T Base::*t) {
  if (std::get<N>(Base::fields).t == t) {
    return &std::get<N>(Base::fields);
  } else {
    return wrapimpl<Base, T, N + 1, M>(t);
  }
}

template <typename Base, typename T, size_t N, size_t M>
    constexpr std::enable_if_t <
    N<M && !hasField<Base, T, N, M>::value, const Nvp<Base, T> *>
    wrapimpl(T Base::*t) {
  return wrapimpl<Base, T, N + 1, M>(t);
}

template <typename Base, typename T, size_t N, size_t M>
constexpr std::enable_if_t<N == M, const Nvp<Base, T> *> wrapimpl(T Base::*t) {
  return nullptr;
}

template <typename Base, typename T>
constexpr const Nvp<Base, T> *wrap(T Base::*t) {
  return wrapimpl<Base, T, 0, std::tuple_size<decltype(Base::fields)>::value>(
      t);
}

template <typename T, T, typename = void> struct hasCallIfFieldIsPresent {};

template <typename Base, typename T, T Base::*ptr>
struct hasCallIfFieldIsPresent<T Base::*, ptr,
                               std::enable_if_t<wrap(ptr) != nullptr>> {
  static constexpr const Nvp<Base, T> &call() { return *wrap(ptr); }
};

MONGO_ODM_INLINE_NAMESPACE_END
} // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
