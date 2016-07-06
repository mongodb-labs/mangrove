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

#include <mongo_odm/model.hpp>
#include <mongo_odm/odm_collection.hpp>
#include <mongo_odm/query_builder.hpp>

using namespace mongocxx;
using namespace mongo_odm;

class Bar : public mongo_odm::model<Bar> {
   public:
    int64_t w;
    int x1;
    int x2;
    bool y;
    std::string z;
    ODM_MAKE_KEYS(Bar, NVP(w), NVP(x1), NVP(x2), NVP(y), NVP(z))

    Bar(int64_t w, int x1, int x2, bool y, std::string z) : w(w), x1(x1), x2(x2), y(y), z(z) {
        _id = bsoncxx::oid{bsoncxx::oid::init_tag_t{}};
    }

    // default constructor
    Bar() {
        _id = bsoncxx::oid{bsoncxx::oid::init_tag_t{}};
    }

    bsoncxx::oid getID() {
        return _id;
    }

    template <class Archive>
    void serialize(Archive &ar) {
        ar(CEREAL_NVP(w), CEREAL_NVP(x1), CEREAL_NVP(x2), CEREAL_NVP(y), CEREAL_NVP(z));
    }
};
ODM_MAKE_KEYS_STORAGE(Bar);

TEST_CASE("Query Builder", "[mongo_odm::query_builder]") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    Bar::setCollection(coll);
    Bar(444, 1, 2, false, "hello").save();
    Bar(444, 1, 3, false, "hello").save();
    Bar(555, 10, 2, true, "goodbye").save();

    SECTION("Test == comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) == 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 == 1);

        res = Bar::find_one(ODM_KEY(Bar::z) == "hello");
        REQUIRE(res);
        REQUIRE(res.value().z == "hello");
    }

    SECTION("Test > comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) > 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 > 1);
    }

    SECTION("Test >= comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) >= 10);
        REQUIRE(res);
        REQUIRE(res.value().x1 >= 10);
    }

    SECTION("Test < comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) < 10);
        REQUIRE(res);
        REQUIRE(res.value().x1 < 10);
    }

    SECTION("Test <= comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) <= 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 <= 1);
    }

    SECTION("Test != comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) != 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 != 1);

        res = Bar::find_one(ODM_KEY(Bar::z) != "hello");
        REQUIRE(res);
        REQUIRE(res.value().z == "goodbye");
    }

    SECTION("Test $not expression, with operator!.", "[mongo_odm::NotExpr]") {
        auto res = Bar::find_one(!(ODM_KEY(Bar::x1) < 10));
        REQUIRE(res);
        REQUIRE(res.value().x1 >= 10);

        res = Bar::find_one(!(ODM_KEY(Bar::z) == "hello"));
        REQUIRE(res);
        REQUIRE(res.value().z == "goodbye");
    }

    SECTION("Test expression list.", "[mongo_odm::ExpressionList]") {
        auto res =
            Bar::find_one((ODM_KEY(Bar::x1) == 1, ODM_KEY(Bar::x2) == 2, ODM_KEY(Bar::w) >= 444));
        REQUIRE(res);
        REQUIRE(res.value().x1 == 1);
        REQUIRE(res.value().x2 == 2);
        REQUIRE(res.value().w >= 444);
    }

    SECTION("Test boolean expressions.", "[mongo_odm::BooleanExpr]") {
        auto res = Bar::find_one(ODM_KEY(Bar::x1) > 9 && ODM_KEY(Bar::x1) < 11);
        REQUIRE(res);
        REQUIRE(res.value().x1 == 10);

        auto cursor = Bar::find(ODM_KEY(Bar::x1) == 10 || ODM_KEY(Bar::x2) == 3);
        int i = 0;
        for (Bar b : cursor) {
            i++;
            // Check that x1 is equal to 10 or x2 is equal to 3
            bool or_test = (b.x1 == 10) || (b.x2 == 3);
            REQUIRE(or_test);
        }
        REQUIRE(i == 2);
    }
}
