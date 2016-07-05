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

#include <iostream>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/cursor.hpp>

#include <bson_mapper/mapping_functions.hpp>

namespace mongo_odm {
MONGO_ODM_INLINE_NAMESPACE_BEGIN

/**
 * A class that wraps a mongocxx::cursor. It provides an iterator that deserializes the
 * documents yielded by the underlying mongocxx cursor.
 * NOTE: This iterator will skip documents that fail to be deserialized, e.g. due to non-matching
 * schemas.
 */
template <class T>
class deserializing_cursor {
   public:
    deserializing_cursor(mongocxx::cursor&& c) : _c(std::move(c)) {
    }

    class iterator;

    iterator begin() {
        return iterator(_c.begin(), _c.end());
    }

    iterator end() {
        return iterator(_c.end(), _c.end());
    }

   private:
    mongocxx::cursor _c;
};

template <class T>
class deserializing_cursor<T>::iterator : public std::iterator<std::input_iterator_tag, T> {
   public:
    iterator(mongocxx::cursor::iterator ci, mongocxx::cursor::iterator ci_end)
        : _ci(ci), _ci_end(ci_end) {
        skip_invalid_documents();
    }

    iterator(const deserializing_cursor::iterator& dsi) : _ci(dsi._ci), _ci_end(dsi._ci_end) {
        skip_invalid_documents();
    }

    iterator& operator++() {
        ++_ci;
        _opt = mongocxx::stdx::nullopt;
        skip_invalid_documents();
        return *this;
    }

    void operator++(int) {
        operator++();
    }

    bool operator==(const iterator& rhs) {
        return _ci == rhs._ci;
    }

    bool operator!=(const iterator& rhs) {
        return _ci != rhs._ci;
    }

    /**
     * Returns a deserialized object that corresponds to the current document pointed to by the
     * underlying collection cursor iterator.
     */
    T operator*() {
        return _opt.value();
    }

   private:
    mongocxx::cursor::iterator _ci;
    // Keeps track of the end of the underlying cursor to enable skipping invalid documents.
    mongocxx::cursor::iterator _ci_end;
    // Cached object value. When this is non-empty, this always contains the current object pointed
    // to by the cursor.
    mongocxx::stdx::optional<T> _opt;

    /**
     * Iterates over documents, and skips documents that cannot be properly deserialized into an
     * object.
     * This stop when a valid document is found, or when the end of the underlying cursor is
     * reached.
     * When a document is successfully converted, it is cached in _opt and used later when
     * dereferencing.
     */
    void skip_invalid_documents() {
        while (_ci != _ci_end) {
            try {
                if (!_opt) {
                    _opt = bson_mapper::to_obj<T>(*_ci);
                }
                return;
            } catch (bson_mapper::Exception& e) {
                ++_ci;
                _opt = mongocxx::stdx::nullopt;
            }
        }
    }
};

MONGO_ODM_INLINE_NAMESPACE_END
}  // namespace bson_mapper

#include <mongo_odm/config/postlude.hpp>
