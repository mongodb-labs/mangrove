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

#include <bsoncxx/builder/stream/document.hpp>

#include <bson_mapper/stdx/optional.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <mongo_odm/model.hpp>

struct DataA : public mongo_odm::model<DataA> {
    int32_t x, y;
    double z;

    DataA() {
        _id = bsoncxx::oid{bsoncxx::oid::init_tag_t{}};
    }

    bool operator==(const DataA& other) {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(_id), CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }

    bsoncxx::oid getID() {
        return _id;
    }
};

TEST_CASE("the model base class successfully allows the saving and removing of records.",
          "[mongo_odm::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mongo_cxx_odm_model_test"];

    DataA::setCollection(db["data_a"]);
    DataA::drop();

    DataA a1;

    a1.x = 16;
    a1.y = 32;
    a1.z = 1.50;

    a1.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "x" << 16
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataA::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(a1 == *query_result);

    query_result->remove();

    REQUIRE(!DataA::find_one(query_filter.view()));
}

TEST_CASE(
    "the model base class successfully provides a cursor to deserialized objects that can be "
    "subsequently updated.",
    "[mongo_odm::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mongo_cxx_odm_model_cursor_test"];
    DataA::setCollection(db["data_a"]);
    DataA::drop();

    for (int i = 0; i < 10; ++i) {
        DataA a1;
        a1.x = a1.y = a1.z = i;
        a1.save();
    }

    // Negate all x values in the collection via pulling them in from a cursor.
    for (auto a : DataA::find({})) {
        a.x *= -1;
        a.save();
    }

    // Sum up the x values in the collection
    int sum = 0;
    for (auto a : DataA::find({})) {
        sum += a.x;
    }

    REQUIRE(sum == -45);
}

using bson_mapper::stdx::optional;

struct DataB : public mongo_odm::model<DataB> {
    int32_t x;
    optional<int32_t> y;
    optional<double> z;

    DataB() {
        _id = bsoncxx::oid{bsoncxx::oid::init_tag_t{}};
    }

    bool operator==(const DataB& other) {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(_id), CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }

    bsoncxx::oid getID() {
        return _id;
    }
};

TEST_CASE(
    "the model base class successfully allows dynamic schemas by way of allowing stdx::optional "
    "elements.",
    "[mongo_odm::model]") {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    auto db = conn["mongo_cxx_odm_model_test"];

    DataB::setCollection(db["data_b"]);
    DataB::drop();

    DataB b1;

    b1.x = 16;
    b1.z = 1.50;

    b1.save();

    auto query_filter = bsoncxx::builder::stream::document{} << "x" << 16
                                                             << bsoncxx::builder::stream::finalize;

    auto query_result = DataB::find_one(query_filter.view());

    REQUIRE(query_result);
    REQUIRE(!query_result->y);
    REQUIRE(b1 == *query_result);

    query_result->remove();

    REQUIRE(!DataB::find_one(query_filter.view()));
}
