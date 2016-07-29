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
#include <mangrove/collection_wrapper.hpp>
#include <mangrove/config/prelude.hpp>
#include <mangrove/util.hpp>
#include <mongocxx/collection.hpp>

namespace mangrove {
MANGROVE_INLINE_NAMESPACE_BEGIN

template <typename T, typename IdType = bsoncxx::oid>
class model {
   private:
// TODO: When XCode 8 is released, this can always be thread_local. Until then, the model class
//       will not be thread-safe on OS X.
#ifdef __APPLE__
    static collection_wrapper<T> _coll;
#else
    static thread_local collection_wrapper<T> _coll;
#endif

   public:
    /**
     * Forward the arguments to the constructor of IdType.
     *
     * A std::enable_if is included to disable the template for the copy constructor case so the
     * default is used.
     *
     * @param ts
     *    The variadic pack of arguments to be forwarded to the constructor of IdType.
     */
    template <typename... Ts,
              typename = std::enable_if_t<!first_two_types_are_same<model, Ts...>::value>>
    model(Ts&&... ts) : _id(std::forward<Ts>(ts)...) {
    }

    model() = default;

    /**
     * Counts the number of documents matching the provided filter.
     *
     *  @param filter
     *    The filter that documents must match in order to be counted.
     *    If a filter is not provided, count() will count the number
     *    of documents in the entire collection.
     *  @param options
     *    Optional arguments, see mongocxx::options::count.
     *
     *  @return The count of the documents that matched the filter.
     *  @throws mongocxx::exception::query if the count operation fails.
     *
     *  @see https://docs.mongodb.com/manual/reference/command/count/
     */
    static std::int64_t count(
        bsoncxx::document::view_or_value filter = bsoncxx::document::view_or_value{},
        const mongocxx::options::count& options = mongocxx::options::count()) {
        return _coll.collection().count(filter, options);
    }

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
     *  Deletes all matching documents from the collection.
     *
     *  @param filter
     *    Document view representing the data to be deleted.
     *  @param options
     *    Optional arguments, see mongocxx::options::delete_options.
     *
     *  @return The optional result of performing the deletion, a mongocxx::result::delete_result.
     *  @throws mongocxx::exception::write if the delete fails.
     *
     *  @see http://docs.mongodb.com/manual/reference/command/delete/
     */
    static mongocxx::stdx::optional<mongocxx::result::delete_result> delete_many(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::delete_options& options = mongocxx::options::delete_options()) {
        return _coll.collection().delete_many(filter, options);
    }

    /**
     *  Deletes a single matching document from the collection.
     *
     *  @param filter
     *    Document view representing the data to be deleted.
     *  @param options
     *    Optional arguments, see mongocxx::options::delete_options.
     *
     *  @return The optional result of performing the deletion, a mongocxx::result::delete_result.
     *  @throws mongocxx::exception::write if the delete fails.
     *
     *  @see http://docs.mongodb.com/manual/reference/command/delete/
     */
    static mongocxx::stdx::optional<mongocxx::result::delete_result> delete_one(
        bsoncxx::document::view_or_value filter,
        const mongocxx::options::delete_options& options = mongocxx::options::delete_options()) {
        return _coll.collection().delete_one(filter, options);
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

    /**
     *  Inserts multiple object of the model into the collection.
     *
     *  @warning This method uses the bulk insert command to execute the insertion as opposed to
     *  the legacy OP_INSERT wire protocol message. As a result, using this method to insert
     *  many documents on MongoDB < 2.6 will be slow.
     *
     *  @tparam containter_type
     *    The container type. Must contain an iterator that yields objects of this model.
     *
     *  @param container
     *    Container of model objects to insert.
     *  @param options
     *    Optional arguments, see mongocxx::options::insert.
     *
     *  @see https://docs.mongodb.com/manual/tutorial/insert-documents/
     *
     *  @return The result of attempting to performing the insert.
     *  @throws mongocxx::exception::write when the operation fails.
     */
    template <typename container_type,
              typename = std::enable_if_t<container_of_v<container_type, T>>>
    static mongocxx::stdx::optional<mongocxx::result::insert_many> insert_many(
        const container_type& container,
        const mongocxx::options::insert& options = mongocxx::options::insert()) {
        return insert_many(container.begin(), container.end(), options);
    }

    /**
     *  Inserts multiple objects of the model into the collection.
     *
     *  @warning This method uses the bulk insert command to execute the insertion as opposed to
     *  the legacy OP_INSERT wire protocol message. As a result, using this method to insert
     *  many documents on MongoDB < 2.6 will be slow.
     *
     *  @tparam object_iterator_type
     *    The iterator type. Must meet the requirements for the input iterator concept with the
     *    model class as the value type.
     *
     *  @param begin
     *    Iterator pointing to the first document to be inserted.
     *  @param end
     *    Iterator pointing to the end of the documents to be inserted.
     *  @param options
     *    Optional arguments, see mongocxx::options::insert.
     *
     *  @see https://docs.mongodb.com/manual/tutorial/insert-documents/
     *
     *  @return The result of attempting to performing the insert.
     *  @throws mongocxx::exception::write if the operation fails.
     *
     */
    template <typename object_iterator_type,
              typename = std::enable_if_t<iterator_of_v<object_iterator_type, T>>>
    static mongocxx::stdx::optional<mongocxx::result::insert_many> insert_many(
        object_iterator_type begin, object_iterator_type end,
        const mongocxx::options::insert& options = mongocxx::options::insert()) {
        return _coll.insert_many(begin, end, options);
    }

    /**
     *  Inserts a single object of the model into the collection.
     *
     *  @param obj
     *    The object of the model to insert.
     *  @param options
     *    Optional arguments, see mongocxx::options::insert.
     *
     *  @see https://docs.mongodb.com/manual/tutorial/insert-documents/
     *
     *  @return The result of attempting to perform the insert.
     *  @throws mongocxx::exception::write if the operation fails.
     */
    static mongocxx::stdx::optional<mongocxx::result::insert_one> insert_one(
        T obj, const mongocxx::options::insert& options = mongocxx::options::insert()) {
        return _coll.insert_one(obj, options);
    }

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
                               << "_id" << this->_id << bsoncxx::builder::stream::finalize;

        _coll.collection().delete_one(id_match_filter.view());
    }

    /**
     * Sets the underlying mongocxx::collection used to store and load instances of T.
     *
     * @param coll The mongocxx::collection object to be mapped to this class.
     *
     * @warning This must be called with a new mongocxx::collection instance for every thread for
     *          the model to be thread-safe.
     *
     * @warning The parent mongocxx::client from which the mongocxx::collection argument was created
     *          must outlive any of this model's CRUD methods. If the client object goes out of
     *          scope, a new collection must be passed to this method before using any CRUD methods.
     */
    static void setCollection(const mongocxx::collection& coll) {
        _coll = collection_wrapper<T>(coll);
    }
    static void setCollection(mongocxx::collection&& coll) {
        _coll = collection_wrapper<T>(std::move(coll));
    }

    /**
     * Performs an update in the database that saves the current T object instance to the
     * collection mapped to this class.
     *
     * In the terms of the CRUD specification, this uses updateOne with the _id as the sole
     * argument to the query filter, the T object serialized to dotted notation BSON as the $set
     * operand, and upsert=true so that objects that aren't already in the collection are
     * automatically inserted.
     *
     * @see https://docs.mongodb.com/manual/reference/method/db.collection.updateOne/
     */
    void save() {
        auto id_match_filter = bsoncxx::builder::stream::document{}
                               << "_id" << this->_id << bsoncxx::builder::stream::finalize;

        auto update = bsoncxx::builder::stream::document{}
                      << "$set" << boson::to_dotted_notation_document(*static_cast<T*>(this))
                      << bsoncxx::builder::stream::finalize;

        mongocxx::options::update opts{};

        opts.upsert(true);

        _coll.collection().update_one(id_match_filter.view(), update.view(), opts);
    };

    /**
     *  Updates multiple documents matching the provided filter in this collection.
     *
     *  @param filter
     *    Document representing the match criteria.
     *  @param update
     *    Document representing the update to be applied to matching documents.
     *  @param options
     *    Optional arguments, see mongocxx::options::update.
     *
     *  @return The result of attempting to update multiple documents.
     *  @throws exception::write if the update operation fails.
     *
     *  @see http://docs.mongodb.com/manual/reference/command/update/
     */
    static mongocxx::stdx::optional<mongocxx::result::update> update_many(
        bsoncxx::document::view_or_value filter, bsoncxx::document::view_or_value update,
        const mongocxx::options::update& options = mongocxx::options::update()) {
        return _coll.collection().update_many(filter, update, options);
    }

    /**
     *  Updates a single document matching the provided filter in this collection.
     *
     *  @param filter
     *    Document representing the match criteria.
     *  @param update
     *    Document representing the update to be applied to a matching document.
     *  @param options
     *    Optional arguments, see mongocxx::options::update.
     *
     *  @return The result of attempting to update a document.
     *  @throws mongocxx::exception::write if the update operation fails.
     *
     *  @see http://docs.mongodb.com/manual/reference/command/update/
     */
    static mongocxx::stdx::optional<mongocxx::result::update> update_one(
        bsoncxx::document::view_or_value filter, bsoncxx::document::view_or_value update,
        const mongocxx::options::update& options = mongocxx::options::update()) {
        return _coll.collection().update_many(filter, update, options);
    }

   protected:
    IdType _id;
};

#ifdef __APPLE__
template <typename T, typename IdType>
collection_wrapper<T> model<T, IdType>::_coll;
#else
template <typename T, typename IdType>
thread_local collection_wrapper<T> model<T, IdType>::_coll;
#endif

MANGROVE_INLINE_NAMESPACE_END
}  // namespace mangrove
