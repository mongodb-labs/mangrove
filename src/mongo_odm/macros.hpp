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

// Preprocessor templates for manipulating multiple arguments.
#define MONGO_ODM_PP_NARG(...) MONGO_ODM_PP_NARG_(__VA_ARGS__, MONGO_ODM_PP_RSEQ_N())
#define MONGO_ODM_PP_NARG_(...) MONGO_ODM_PP_ARG_N(__VA_ARGS__)
#define MONGO_ODM_PP_ARG_N(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, \
                           _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, \
                           _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, \
                           _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, \
                           _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, _71, \
                           _72, _73, _74, _75, _76, _77, _78, _79, _80, _81, _82, _83, _84, _85, \
                           _86, _87, _88, _89, _90, _91, _92, _93, _94, _95, _96, _97, _98, _99, \
                           _100, _101, N, ...)                                                   \
    N

#define MONGO_ODM_PP_RSEQ_N()                                                                      \
    102, 101, 100, 99, 98, 97, 96, 95, 94, 93, 92, 91, 90, 89, 88, 87, 86, 85, 84, 83, 82, 81, 80, \
        79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58,    \
        57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36,    \
        35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14,    \
        13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

#define MONGO_ODM_PASTE_IMPL(s1, s2) s1##s2
#define MONGO_ODM_PASTE(s1, s2) MONGO_ODM_PASTE_IMPL(s1, s2)

// Wrap a field name to create a corresponding NVP
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
#define MONGO_ODM_KEY_BY_VALUE(value) \
    mongo_odm::hasCallIfFieldIsPresent<decltype(value), value>::call()

#define MONGO_ODM_CHILD1(base, field1) MONGO_ODM_KEY_BY_VALUE(&base::field1)

#define MONGO_ODM_CHILD2(base, field1, field2)                                               \
    makeNvpWithParent(                                                                       \
        MONGO_ODM_KEY_BY_VALUE(                                                              \
            &std::decay_t<typename decltype(MONGO_ODM_CHILD1(base, field1))::type>::field2), \
        MONGO_ODM_CHILD1(base, field1))

#define MONGO_ODM_CHILD3(base, field1, field2, field3)                                         \
    makeNvpWithParent(MONGO_ODM_KEY_BY_VALUE(&std::decay_t<typename decltype(MONGO_ODM_CHILD2( \
                                                 base, field1, field2))::type>::field3),       \
                      MONGO_ODM_CHILD2(base, field1, field2))

#define MONGO_ODM_CHILD(type, ...) \
    MONGO_ODM_PASTE(MONGO_ODM_CHILD, MONGO_ODM_PP_NARG(__VA_ARGS__))(type, __VA_ARGS__)

#include <mongo_odm/config/postlude.hpp>
