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

#include "catch.hpp"

#include <iostream>

#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pipeline.hpp>
#include <mongocxx/stdx.hpp>

#include <boson/bson_streambuf.hpp>
#include <mangrove/collection_wrapper.hpp>

using namespace bsoncxx;
using namespace mongocxx;
using namespace mangrove;

class Foo {
   public:
    int a, b, c;

    bool operator==(const Foo& rhs) {
        return (a == rhs.a) && (b == rhs.b) && (c == rhs.c);
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(a), CEREAL_NVP(b), CEREAL_NVP(c));
    }
};

TEST_CASE("Test deserializing cursor", "[mangrove::deserializing_cursor]") {
    // set up test BSON documents and objects
    std::string json_str = R"({"a": 1, "b":4, "c": 9})";
    auto doc = from_json(json_str);
    auto doc_view = doc.view();

    std::string json_str_2 = R"({"a": 1, "b":4, "c": 900})";
    auto doc_2 = from_json(json_str_2);
    auto doc_2_view = doc_2.view();

    Foo obj{1, 4, 9};

    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    collection_wrapper<Foo> foo_coll(coll);

    SECTION("Deserializing cursor automatically converts documents from cursors to objects.",
            "[mangrove::deserializing_cursor]") {
        coll.delete_many({});

        for (int i = 0; i < 5; i++) {
            coll.insert_one(doc_view);
            coll.insert_one(doc_2_view);
        }

        deserializing_cursor<Foo> cur = foo_coll.find(from_json(R"({"c": {"$gt": 100}})"));
        int i = 0;
        for (Foo f : cur) {
            REQUIRE(f.c > 100);
            i++;
        }
        REQUIRE(i == 5);
    }

    SECTION("Deserializing cursor skips invalid documents.", "[mangrove::deserializing_cursor]") {
        coll.delete_many({});
        // populate collection with documents, some of which don't match the Foo's schema
        // Also specify id's so that we can have invalid documents at the end and beginning
        // of the iterator, for testing.
        coll.insert_one(from_json(R"({"_id": 1, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 2, "a": 100, "b": 200, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 3, "a": 100, "b": 200, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 4, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 5, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 6, "a": 100, "b": 200, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 7, "a": 100, "b": 200, "c": 900})"));
        coll.insert_one(from_json(R"({"_id": 8, "c": 900})"));

        // set up sort options.
        mongocxx::options::find opts;
        opts.sort(from_json(R"({"_id": 1})"));

        auto filter = from_json(R"({"c": {"$gt": 100}})");
        deserializing_cursor<Foo> cur = foo_coll.find(filter.view(), opts);
        int i = 0;
        for (Foo f : cur) {
            REQUIRE(f.c > 100);
            i++;
        }
        REQUIRE(i == 4);
    }

    coll.delete_many({});
}
