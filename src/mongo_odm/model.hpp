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

#include <cereal/cereal.hpp>

#include <bsoncxx/oid.hpp>
#include <mongo_odm/config/prelude.hpp>
#include <mongo_odm/odm_collection.hpp>
#include <mongocxx/collection.hpp>

#define NVP(x) mongo_odm::makeNvp(&mongo_odm_wrap_base::x, #x)

// Creates serialize() function
#define ODM_SERIALIZE_KEYS                                                           \
    template <class Archive>                                                         \
    void serialize(Archive& ar) {                                                    \
        serialize_recur<Archive, 0, std::tuple_size<decltype(fields())>::value>(ar); \
    }                                                                                \
    template <class Archive, size_t I, size_t N>                                     \
    typename std::enable_if<(I < N), void>::type serialize_recur(Archive& ar) {      \
        auto nvp = std::get<I>(fields());                                            \
        ar(cereal::make_nvp(nvp.name, this->*(nvp.t)));                              \
        serialize_recur<Archive, I + 1, N>(ar);                                      \
    }                                                                                \
                                                                                     \
    template <class Archive, size_t I, size_t N>                                     \
    typename std::enable_if<(I == N), void>::type serialize_recur(Archive& ar) {     \
        ;                                                                            \
    }

// Register members and create serialize() function
#define ODM_MAKE_KEYS(Base, ...)             \
    using mongo_odm_wrap_base = Base;        \
    constexpr static auto fields() {         \
        return std::make_tuple(__VA_ARGS__); \
    }                                        \
    ODM_SERIALIZE_KEYS

// If using the mongo_odm::model, then also register _id as a field.
#define ODM_MAKE_KEYS_MODEL(Base, ...) ODM_MAKE_KEYS(Base, NVP(_id), __VA_ARGS__)

#define ODM_KEY(value) mongo_odm::hasCallIfFieldIsPresent<decltype(&value), &value>::call()

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

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

    T Base::*t;
    const char* name;
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
    : public std::is_same<T Base::*, decltype(std::get<N>(Base::fields()).t)> {};

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
    if (std::get<N>(Base::fields()).t == t) {
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
    if (std::get<N>(Base::fields()).t == t) {
        return std::get<N>(Base::fields());
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
    return wrapimpl<Base, T, 0, std::tuple_size<decltype(Base::fields())>::value>(t);
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
    std::enable_if_t<wrapbool<Base, T, 0, std::tuple_size<decltype(Base::fields())>::value>(ptr)>> {
    static constexpr const Nvp<Base, T> call() {
        return wrap(ptr);
    }
};

template <typename T, typename IdType = bsoncxx::oid>
class model {
   private:
// TODO: When XCode 8 is released, this can always be thread_local. Until then, the model class
//       will not be thread-safe on OS X.
#ifdef __APPLE__
    static odm_collection<T> _coll;
#else
    static thread_local odm_collection<T> _coll;
#endif

   protected:
    // TODO: if IdType is bsoncxx::oid, the derived class must initialize this until CXX-940 is
    //       resolved.
    //
    // If IdType is not bsoncxx::oid, the derived class is always responsible for initializing it.
    IdType _id;

   public:
    /**
     * Returns a copy of the underlying collection.
     *
     * @return A copy of the underlying mongocxx::collection that this class uses to store and
     * load instances of T.
     */
    static const mongocxx::collection collection() {
        return _coll.collection();
    }

    /**
     * Drops the underlying collection and all its contained documents from the database.
     *
     * @throws exception::operation if the operation fails.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.drop/
     */
    static void drop() {
        _coll.collection().drop();
    }

    /**
     * Sets the underlying mongocxx::collection used to store and load instances of T.
     *
     * @param coll The mongocxx::collection object to be mapped to this class.
     *
     * @warning This must be called with a new mongocxx::collection instance for every thread for
     *          the ODM to be thread-safe.
     */
    static void setCollection(const mongocxx::collection& coll) {
        _coll = odm_collection<T>(coll);
    }
    static void setCollection(mongocxx::collection&& coll) {
        _coll = odm_collection<T>(std::move(coll));
    }

    /**
     * Performs an update in the database that saves the current T object instance to the
     * collection mapped to this class.
     *
     * In the terms of the CRUD specification, this uses replaceOne with the _id as the sole
     * argument to the query filter and upsert=true so that objects that aren't already in the
     * collection are automatically inserted.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.replaceOne/
     *
     * @warning This will fully replace any object in the database with an _id equal to
     *          this instance's _id.
     */
    void save() {
        auto id_match_filter = bsoncxx::builder::stream::document{}
                               << "_id" << _id << bsoncxx::builder::stream::finalize;
        mongocxx::options::update opts{};
        opts.upsert(true);

        _coll.replace_one(id_match_filter.view(), *static_cast<T*>(this), opts);
    };

    /**
     * Deletes this object from the underlying collection.
     *
     * In the terms of the CRUD specification, this uses deleteOne with the _id as the sole
     * argument to the query filter.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.deleteOne/
     */
    void remove() {
        auto id_match_filter = bsoncxx::builder::stream::document{}
                               << "_id" << _id << bsoncxx::builder::stream::finalize;

        _coll.collection().delete_one(id_match_filter.view());
    }

    /**
     * Finds the documents in this collection which match the provided filter.
     *
     * @param filter
     *   Document view representing a document that should match the query.
     * @param options
     *   Optional arguments, see mongocxx::options::find
     *
     * @return Cursor with deserialized objects from the collection.
     * @throws
     *   If the find failed, the returned cursor will throw mongocxx::exception::query when it
     *   is iterated.
     *
     * @see https://docs.mongodb.com/manual/tutorial/query-documents/
     */
    static deserializing_cursor<T> find(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find& options = mongocxx::options::find()) {
        return _coll.find(std::move(filter), options);
    }

    /**
     * Finds a single document in this collection that matches the provided filter.
     *
     * @param filter
     *   Document view representing a document that should match the query.
     * @param options
     *   Optional arguments, see mongocxx::options::find
     *
     * @return An optional object that matched the filter.
     * @throws mongocxx::exception::query if the operation fails.
     *
     * @see https://docs.mongodb.com/manual/tutorial/query-documents/
     */
    static mongocxx::stdx::optional<T> find_one(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::find& options = mongocxx::options::find()) {
        return _coll.find_one(std::move(filter), options);
    }
};

template <typename T, typename IdType>
odm_collection<T> model<T, IdType>::_coll;

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace mongo_odm
