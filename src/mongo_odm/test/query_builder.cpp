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
    MONGO_ODM_MAKE_KEYS_MODEL(Bar, MONGO_ODM_NVP(w), MONGO_ODM_NVP(x1), MONGO_ODM_NVP(x2),
                              MONGO_ODM_NVP(y), MONGO_ODM_NVP(z))

    Bar(int64_t w, int x1, int x2, bool y, std::string z) : w(w), x1(x1), x2(x2), y(y), z(z) {
    }

    // default constructor
    Bar() = default;

    bsoncxx::oid getID() {
        return _id;
    }
};

// Test ODM on class that does not rely on model
class Point {
   public:
    int x;
    int y;
    MONGO_ODM_MAKE_KEYS(Point, MONGO_ODM_NVP(x), MONGO_ODM_NVP(y));
};

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
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) == 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 == 1);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) == "hello");
        REQUIRE(res);
        REQUIRE(res.value().z == "hello");
    }

    SECTION("Test > comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) > 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 > 1);
    }

    SECTION("Test >= comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) >= 10);
        REQUIRE(res);
        REQUIRE(res.value().x1 >= 10);
    }

    SECTION("Test < comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) < 10);
        REQUIRE(res);
        REQUIRE(res.value().x1 < 10);
    }

    SECTION("Test <= comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) <= 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 <= 1);
    }

    SECTION("Test != comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) != 1);
        REQUIRE(res);
        REQUIRE(res.value().x1 != 1);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) != "hello");
        REQUIRE(res);
        REQUIRE(res.value().z == "goodbye");
    }

    SECTION("Test $not expression, with operator!.", "[mongo_odm::NotExpr]") {
        auto res = Bar::find_one(!(MONGO_ODM_KEY(Bar::x1) < 10));
        REQUIRE(res);
        REQUIRE(res.value().x1 >= 10);

        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::z) == "hello"));
        REQUIRE(res);
        REQUIRE(res.value().z == "goodbye");
    }

    SECTION("Test expression list.", "[mongo_odm::ExpressionList]") {
        auto res = Bar::find_one((MONGO_ODM_KEY(Bar::x1) == 1, MONGO_ODM_KEY(Bar::x2) == 2,
                                  MONGO_ODM_KEY(Bar::w) >= 444));
        REQUIRE(res);
        REQUIRE(res.value().x1 == 1);
        REQUIRE(res.value().x2 == 2);
        REQUIRE(res.value().w >= 444);
    }

    SECTION("Test boolean expressions.", "[mongo_odm::BooleanExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) > 9 && MONGO_ODM_KEY(Bar::x1) < 11);
        REQUIRE(res);
        REQUIRE(res.value().x1 == 10);

        auto cursor = Bar::find(MONGO_ODM_KEY(Bar::x1) == 10 || MONGO_ODM_KEY(Bar::x2) == 3);
        int i = 0;
        for (Bar b : cursor) {
            i++;
            // Check that x1 is equal to 10 or x2 is equal to 3
            bool or_test = (b.x1 == 10) || (b.x2 == 3);
            REQUIRE(or_test);
        }
        REQUIRE(i == 2);

        // Test a complex boolean expression, with parentheses and mixed oeprators
        res = Bar::find_one(
            (MONGO_ODM_KEY(Bar::z) == "goodbye" || !(MONGO_ODM_KEY(Bar::y) == false)) &&
            (MONGO_ODM_KEY(Bar::w) == 555 || MONGO_ODM_KEY(Bar::x2) == 3));
        REQUIRE(res);
        REQUIRE(res.value().z == "goodbye");
    }
}

TEST_CASE("Query builder works with non-ODM class") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    auto point_coll = odm_collection<Point>(coll);
    point_coll.insert_one({5, 6});
    auto res = point_coll.find_one(MONGO_ODM_KEY(Point::x) == 5);
    REQUIRE(res.value().x == 5);

    coll.delete_many({});
}

TEST_CASE("Update Builder", "mongo_odm::UpdateExpr") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    Bar::setCollection(coll);
    Bar(444, 1, 2, false, "hello").save();
    Bar(444, 1, 3, false, "hello").save();
    Bar(555, 10, 2, true, "goodbye").save();

    SECTION("Test = assignment.", "[mongo_odm::UpdateExpr]") {
        auto res =
            coll.update_one(ODM_KEY(Bar::w) == 555, (ODM_KEY(Bar::x1) = 73, ODM_KEY(Bar::x2) = 99));
        REQUIRE(res);
        REQUIRE(res.value().modified_count() == 1);
        auto bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar.value().x1 == 73);
        REQUIRE(bar.value().x2 == 99);
    }

    SECTION("Test increment operators.", "[mongo_odm::UpdateExpr]") {
        auto bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        int initial_x1 = bar.value().x1;

        {
            // Test postfix ++
            auto res = coll.update_one(ODM_KEY(Bar::w) == 555, ODM_KEY(Bar::x1)++);
            REQUIRE(res);
            REQUIRE(res.value().modified_count() == 1);
            bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar.value().x1 == initial_x1 + 1);
        }

        {
            // Test prefix ++
            initial_x1 = bar.value().x1;
            auto res = coll.update_one(ODM_KEY(Bar::w) == 555, ++ODM_KEY(Bar::x1));
            REQUIRE(res);
            REQUIRE(res.value().modified_count() == 1);
            bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar.value().x1 == initial_x1 + 1);
        }

        {
            // Test postfix --
            initial_x1 = bar.value().x1;
            auto res = coll.update_one(ODM_KEY(Bar::w) == 555, ODM_KEY(Bar::x1)--);
            REQUIRE(res);
            REQUIRE(res.value().modified_count() == 1);
            bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar.value().x1 == initial_x1 - 1);
        }

        {
            // Test prefix --
            initial_x1 = bar.value().x1;
            auto res = coll.update_one(ODM_KEY(Bar::w) == 555, --ODM_KEY(Bar::x1));
            REQUIRE(res);
            REQUIRE(res.value().modified_count() == 1);
            bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar.value().x1 == initial_x1 - 1);
        }

        {
            // Test operator+=
            initial_x1 = bar.value().x1;
            auto res = coll.update_one(ODM_KEY(Bar::w) == 555, ODM_KEY(Bar::x1) += 5);
            REQUIRE(res);
            REQUIRE(res.value().modified_count() == 1);
            bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar.value().x1 == initial_x1 + 5);
        }

        {
            // Test operator-=
            initial_x1 = bar.value().x1;
            auto res = coll.update_one(ODM_KEY(Bar::w) == 555, ODM_KEY(Bar::x1) -= 5);
            REQUIRE(res);
            REQUIRE(res.value().modified_count() == 1);
            bar = Bar::find_one(ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar.value().x1 == initial_x1 - 5);
        }
    }
}
