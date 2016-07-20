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

#include <cereal/types/vector.hpp>

#include <bsoncxx/builder/stream/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pipeline.hpp>
#include <mongocxx/stdx.hpp>

#include <mongo_odm/model.hpp>
#include <mongo_odm/odm_collection.hpp>
#include <mongo_odm/query_builder.hpp>

#include <bsoncxx/types.hpp>

using namespace std::chrono;
using namespace mongocxx;

// An ODM class that does not rely on model
class Point {
   public:
    int x;
    int y;
    MONGO_ODM_MAKE_KEYS(Point, MONGO_ODM_NVP(x), MONGO_ODM_NVP(y));

    bool operator==(const Point& rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
};

// An ODM class that inherits from model
class Bar : public mongo_odm::model<Bar> {
   public:
    int64_t w;
    int x1;
    stdx::optional<int> x2;
    bool y;
    std::string z;
    Point p;
    std::vector<int> arr;
    std::vector<Point> pts;
    system_clock::time_point t;

    MONGO_ODM_MAKE_KEYS_MODEL(Bar, MONGO_ODM_NVP(w), MONGO_ODM_NVP(x1), MONGO_ODM_NVP(x2),
                              MONGO_ODM_NVP(y), MONGO_ODM_NVP(z), MONGO_ODM_NVP(p),
                              MONGO_ODM_NVP(arr), MONGO_ODM_NVP(pts), MONGO_ODM_NVP(t));

    Bar(int64_t w, int x1, stdx::optional<int> x2, bool y, std::string z, Point p,
        std::vector<int> arr, std::vector<Point> pts, system_clock::time_point t)
        : w(w), x1(x1), x2(x2), y(y), z(z), p(p), arr(arr), pts(pts), t(t) {
    }

    // default constructor
    Bar() = default;

    bsoncxx::oid getID() {
        return _id;
    }
};

class BarParent {
   public:
    Bar b;

    BarParent() {
    }

    // default constructor
    BarParent(Bar b) : b(b) {
    }
    MONGO_ODM_MAKE_KEYS(BarParent, MONGO_ODM_NVP(b));
};

TEST_CASE("Test member access.", "[mongo_odm::Nvp]") {
    SECTION("Member access using MONGO_ODM_KEY") {
        auto nvp = MONGO_ODM_KEY(Bar::x1);
        REQUIRE(nvp.get_name() == "x1");
        REQUIRE(nvp.t == &Bar::x1);
    }

    SECTION("Member access using MONGO_ODM_CHILD macro") {
        auto nvp = MONGO_ODM_CHILD(Bar, p);
        REQUIRE(nvp.get_name() == "p");
        REQUIRE(nvp.t == &Bar::p);
    }
}

TEST_CASE("Test nested member access.", "[mongo_odm::NvpChild]") {
    SECTION("Nested member access using operator->*") {
        auto nvp_child = MONGO_ODM_KEY(Bar::p)->*MONGO_ODM_KEY(Point::x);
        REQUIRE(nvp_child.get_name() == "p.x");
        REQUIRE(nvp_child.t == &Point::x);
        REQUIRE(nvp_child.parent.get_name() == "p");
        REQUIRE(nvp_child.parent.t == &Bar::p);
    }

    SECTION("Multiple nested member access using operator->*") {
        auto nvp_child =
            MONGO_ODM_KEY(BarParent::b)->*MONGO_ODM_KEY(Bar::p)->*MONGO_ODM_KEY(Point::x);
        REQUIRE(nvp_child.get_name() == "b.p.x");
        REQUIRE(nvp_child.t == &Point::x);
        REQUIRE(nvp_child.parent.get_name() == "b.p");
        REQUIRE(nvp_child.parent.t == &Bar::p);
        REQUIRE(nvp_child.parent.parent.get_name() == "b");
        REQUIRE(nvp_child.parent.parent.t == &BarParent::b);
    }

    SECTION("Nested member access using MONGO_ODM_CHILD macro") {
        auto nvp_child = MONGO_ODM_CHILD(Bar, p, x);
        REQUIRE(nvp_child.get_name() == "p.x");
        REQUIRE(nvp_child.t == &Point::x);
        REQUIRE(nvp_child.parent.get_name() == "p");
        REQUIRE(nvp_child.parent.t == &Bar::p);
    }

    SECTION("Multiple nested member access using MONGO_ODM_CHILD macro") {
        auto nvp_child = MONGO_ODM_CHILD(BarParent, b, p, x);
        REQUIRE(nvp_child.get_name() == "b.p.x");
        REQUIRE(nvp_child.t == &Point::x);
        REQUIRE(nvp_child.parent.get_name() == "b.p");
        REQUIRE(nvp_child.parent.t == &Bar::p);
        REQUIRE(nvp_child.parent.parent.get_name() == "b");
        REQUIRE(nvp_child.parent.parent.t == &BarParent::b);
    }
}

TEST_CASE("Query Builder", "[mongo_odm::query_builder]") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    Bar::setCollection(coll);
    Bar(444, 1, 2, false, "hello", {0, 0}, {4, 5, 6}, {{1, 2}, {3, 4}}, system_clock::now()).save();
    Bar(444, 1, 3, false, "hello", {0, 1}, {4, 5, 6}, {{5, 6}, {7, 8}}, system_clock::now()).save();
    Bar(555, 10, 2, true, "goodbye", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
        .save();

    SECTION("Test == comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) == 1);
        REQUIRE(res);
        REQUIRE(res->x1 == 1);

        // Test optional type
        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2) == 3);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) == "hello");
        REQUIRE(res);
        REQUIRE(res->z == "hello");

        // nested member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::p)->*MONGO_ODM_KEY(Point::x) == 1);
        REQUIRE(res);
        REQUIRE(res->p.x == 1);

        // Complex type test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::p) == Point{0, 1});
        REQUIRE(res);
        REQUIRE((res->p == Point{0, 1}));

        // scalar array (vector) member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::arr) == std::vector<int>{4, 5, 6});
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::pts) == std::vector<Point>{{1, 2}, {3, 4}});
        REQUIRE(res);
        REQUIRE((res->pts == std::vector<Point>{{1, 2}, {3, 4}}));
    }

    SECTION("Test > comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) > 1);
        REQUIRE(res);
        REQUIRE(res->x1 > 1);

        // Test optional type
        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2) > 2);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        // Test lexicographical comparison
        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) > "goodbye");
        REQUIRE(res);
        REQUIRE(res->z == "hello");

        // nested member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::p)->*MONGO_ODM_KEY(Point::x) > 0);
        REQUIRE(res);
        REQUIRE(res->p.x > 0);

        // Complex type test, lexicographical comparison
        res = Bar::find_one(MONGO_ODM_KEY(Bar::p) > Point{0, 1});
        REQUIRE(res);
        REQUIRE((res->p == Point{1, 0}));

        // scalar array (vector) member test (lexicographical comparison)
        res = Bar::find_one(MONGO_ODM_KEY(Bar::arr) > std::vector<int>{1, 2, 3});
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::pts) > std::vector<Point>{{5, 6}, {7, 8}});
        REQUIRE(res);
        REQUIRE((res->pts == std::vector<Point>{{9, 10}, {11, 12}}));
    }

    SECTION("Test >= comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) >= 10);
        REQUIRE(res);
        REQUIRE(res->x1 >= 10);

        // Test optional type
        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2) >= 3);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        // Test lexicographical comparison
        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) >= "hello");
        REQUIRE(res);
        REQUIRE(res->z == "hello");
    }

    SECTION("Test < comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) < 10);
        REQUIRE(res);
        REQUIRE(res->x1 < 10);

        // Test optional type
        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2) < 3);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 2);

        // Test lexicographical comparison
        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) < "hello");
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test <= comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) <= 1);
        REQUIRE(res);
        REQUIRE(res->x1 <= 1);

        // Test optional type
        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2) <= 2);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 2);

        // Test lexicographical comparison
        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) <= "goodbye");
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test != comparison.", "[mongo_odm::ComparisonExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) != 1);
        REQUIRE(res);
        REQUIRE(res->x1 != 1);

        // Test optional type
        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2) != 2);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::z) != "hello");
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // Complex type test
        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::p) < Point{1, 0}));
        REQUIRE(res);
        REQUIRE((res->p == Point{1, 0}));

        // nested member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::p)->*MONGO_ODM_KEY(Point::x) != 0);
        REQUIRE(res);
        REQUIRE(res->p.x != 0);
    }

    SECTION("Test $not expression, with operator!.", "[mongo_odm::NotExpr]") {
        auto res = Bar::find_one(!(MONGO_ODM_KEY(Bar::x1) < 10));
        REQUIRE(res);
        REQUIRE(res->x1 >= 10);

        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::z) == "hello"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // Test optional type
        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::x2) <= 2));
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        // nested member test
        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::p)->*MONGO_ODM_KEY(Point::x) == 1));
        REQUIRE(res);
        REQUIRE(res->p.x != 1);

        // array (vector) member test
        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::arr) == std::vector<int>{1, 5, 6}));
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(!(MONGO_ODM_KEY(Bar::pts) == std::vector<Point>{{1, 2}, {3, 4}}));
        REQUIRE(res);
        REQUIRE((res->pts != std::vector<Point>{{1, 2}, {3, 4}}));
    }

    SECTION("Test $nin and $in operators.", "[mongo_odm::InArrayExpr]") {
        std::vector<int> nums;
        nums.push_back(1);
        nums.push_back(2);
        nums.push_back(555);

        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::w).in(nums));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2).in(nums));
        REQUIRE(res);
        REQUIRE(res->x2 == 2);

        nums.clear();
        nums.push_back(1);
        nums.push_back(2);
        nums.push_back(444);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::w).nin(nums));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        res = Bar::find_one(MONGO_ODM_KEY(Bar::x2).nin(nums));
        REQUIRE(res);
        REQUIRE(res->x2 == 3);

        res = Bar::find_one(MONGO_ODM_CHILD(Bar, p, y).nin(nums) && MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(res);
        REQUIRE(res->w == 555);
        REQUIRE(res->p.y == 0);

        // Test query builder with arrays of complex values
        res = Bar::find_one(MONGO_ODM_KEY(Bar::p).in(std::vector<Point>{{0, 1}, {0, 2}}));
        REQUIRE(res);
        REQUIRE((res->p == Point{0, 1}));

        // scalar array (vector) member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::arr).in(std::vector<int>{1, 2, 6}));
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(MONGO_ODM_KEY(Bar::pts).in(std::vector<Point>{{1, 2}, {3, 4}}));
        REQUIRE(res);
        REQUIRE((res->pts == std::vector<Point>{{1, 2}, {3, 4}}));
    }

    SECTION("Test expression list.", "[mongo_odm::ExpressionList]") {
        auto res = Bar::find_one((MONGO_ODM_KEY(Bar::x1) == 1, MONGO_ODM_KEY(Bar::x2) == 2,
                                  MONGO_ODM_KEY(Bar::w) >= 444, MONGO_ODM_CHILD(Bar, p, y) == 0));
        REQUIRE(res);
        REQUIRE(res->x1 == 1);
        REQUIRE(res->x2 == 2);
        REQUIRE(res->w >= 444);
        REQUIRE(res->p.x == 0);

        // Test variadic ExpressionList builder. ODM clients shouldn't need to use this,
        // but it's used internally by functions such as nor(...) that accept variadic arguments.
        res = Bar::find_one(mongo_odm::make_expression_list(
            MONGO_ODM_KEY(Bar::x1) == 1, MONGO_ODM_KEY(Bar::x2) == 2, MONGO_ODM_KEY(Bar::w) >= 444,
            MONGO_ODM_CHILD(Bar, p, y) == 0));
        REQUIRE(res);
        REQUIRE(res->x1 == 1);
        REQUIRE(res->x2 == 2);
        REQUIRE(res->w >= 444);
        REQUIRE(res->p.x == 0);
    }

    SECTION("Test boolean expressions.", "[mongo_odm::BooleanExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1) > 9 && MONGO_ODM_KEY(Bar::x1) < 11 &&
                                 MONGO_ODM_CHILD(Bar, p, x) > 0);
        REQUIRE(res);
        REQUIRE(res->x1 == 10);
        REQUIRE(res->x1 == 10);

        auto cursor = Bar::find(MONGO_ODM_KEY(Bar::x1) == 10 || MONGO_ODM_KEY(Bar::x2) == 3 ||
                                MONGO_ODM_CHILD(Bar, p, x) == 1);
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
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test boolean operators on expression lists. (used primarily for $nor)",
            "[mongo_odm::BooleanListExpr]") {
        auto res =
            Bar::find_one(nor(MONGO_ODM_KEY(Bar::w) == 444, MONGO_ODM_KEY(Bar::x1) == 1,
                              MONGO_ODM_KEY(Bar::x2) == 3, MONGO_ODM_KEY(Bar::y) == false,
                              MONGO_ODM_KEY(Bar::z) == "hello", MONGO_ODM_CHILD(Bar, p, x) == 0));
        REQUIRE(res);
        Bar b = res.value();
        REQUIRE(b.w != 444);
        REQUIRE(b.x1 != 1);
        REQUIRE(b.x2 != 3);
        REQUIRE(b.y != false);
        REQUIRE(b.z != "hello");
        REQUIRE(b.p.x != 0);
    }

    SECTION("Test $exists operator on optional fields.", "[mongo_odm::Nvp::exists]") {
        auto count = coll.count(MONGO_ODM_KEY(Bar::x2).exists(true));
        REQUIRE(count == 3);

        Bar(444, 1, stdx::nullopt, false, "hello", {0, 0}, {4, 5, 6}, {{1, 2}}, system_clock::now())
            .save();
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x2).exists(false));
        REQUIRE(res);
        REQUIRE(!res->x2);
    }

    SECTION("Test $mod operator.", "[mongo_odm::ModExpr]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::w).mod(50, 5));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        // Not Expressions can contain Mod Expressions
        res = Bar::find_one(!MONGO_ODM_KEY(Bar::w).mod(50, 5));
        REQUIRE(res);
        REQUIRE(res->w == 444);

        res = Bar::find_one(!!MONGO_ODM_KEY(Bar::w).mod(50, 5));
        REQUIRE(res);
        REQUIRE(res->w == 555);
    }

    SECTION("Test $text operator.", "[mongo_odm::TextSearchExpr]") {
        auto value = bsoncxx::from_json(R"({"z": "text"})");
        coll.create_index(value.view());

        auto res = Bar::find_one(mongo_odm::text("goodbye"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        res = Bar::find_one(mongo_odm::text("goodbye", "en"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test $regex operator.", "[mongo_odm::Nvp::regex]") {
        auto res = Bar::find_one(MONGO_ODM_KEY(Bar::z).regex("o+d", ""));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        res = Bar::find_one(MONGO_ODM_KEY(Bar::z).regex("O+D", "i"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // Test $not with regex.
        res = Bar::find_one(!MONGO_ODM_KEY(Bar::z).regex("O+D", "i"));
        REQUIRE(res);
        REQUIRE(res->z == "hello");

        res = Bar::find_one(!!MONGO_ODM_KEY(Bar::z).regex("O+D", "i"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test array query operators") {
        SECTION("Test $size operator.", "[mongo_odm::Nvp::size]") {
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::arr).size(3));
            REQUIRE(res);
            REQUIRE(res->arr.size() == 3);
        }

        SECTION("Test $all operator.", "[mongo_odm::Nvp::all]") {
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::arr).all(std::vector<int>{4, 5, 6}));
            REQUIRE(res);
            REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

            res = Bar::find_one(MONGO_ODM_KEY(Bar::arr).all(std::vector<int>{4, 5, 6, 7}));
            REQUIRE(!res);
        }

        SECTION("Test $elem_match operator.", "[mongo_odm::Nvp::elem_match]") {
            // Test scalar array
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::arr).elem_match(
                (mongo_odm::free_expr(MONGO_ODM_KEY(Bar::arr).element() > 5),
                 mongo_odm::free_expr(MONGO_ODM_KEY(Bar::arr).element() < 7))));
            REQUIRE(res);
            REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

            // Test scalar array with $not
            res = Bar::find_one(!MONGO_ODM_KEY(Bar::arr).elem_match(
                mongo_odm::free_expr(MONGO_ODM_KEY(Bar::arr).element() > 6)));
            REQUIRE(res);
            REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

            // Test document array
            res = Bar::find_one(MONGO_ODM_KEY(Bar::pts).elem_match(
                (MONGO_ODM_KEY(Point::x) > 7, MONGO_ODM_KEY(Point::y) > 8)));
            REQUIRE(res);
            REQUIRE((res->pts == std::vector<Point>{{9, 10}, {11, 12}}));

            // Test document array with $not
            res = Bar::find_one(!MONGO_ODM_KEY(Bar::pts).elem_match(
                (MONGO_ODM_KEY(Point::x) <= 7, MONGO_ODM_KEY(Point::y) <= 8)));
            REQUIRE(res);
            REQUIRE((res->pts == std::vector<Point>{{9, 10}, {11, 12}}));
        }
    }

    SECTION("Test bitwise query operators") {
        SECTION("Test $bitsAllSet operators.", "[mongo_odm::Nvp::bits_all_set]") {
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1).bits_all_set(10));
            REQUIRE(res);
            REQUIRE(res->x1 == 10);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::x1).bits_all_set(1, 3));
            REQUIRE(res);
            REQUIRE(res->x1 == 10);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_all_set(1, 2, 3, 4));
            REQUIRE(!res);
        }

        SECTION("Test $bitsAnySet operators.", "[mongo_odm::Nvp::bits_any_set]") {
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::x1).bits_any_set(10));
            REQUIRE(res);
            REQUIRE((res->x1 | 10) > 0);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::x1).bits_any_set(1, 3));
            REQUIRE(res);
            REQUIRE((res->x1 | (1 << 1) | (1 << 3)) > 0);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::x1).bits_any_set(12, 13));
            REQUIRE(!res);
        }

        SECTION("Test $bitsAllClear operators.", "[mongo_odm::Nvp::bits_all_clear]") {
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_all_clear((~555) & 0xFFFF));
            REQUIRE(res);
            REQUIRE(res->w == 555);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_all_clear(2, 4, 6, 7, 8));
            REQUIRE(res);
            REQUIRE(res->w == 555);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_all_clear(0, 1, 2, 3, 4, 5, 6));
            REQUIRE(!res);
        }

        SECTION("Test $bitsAnyClear operators.", "[mongo_odm::Nvp::bits_any_clear]") {
            auto res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_any_clear(21));
            REQUIRE(res);
            REQUIRE((res->w & 21) < 21);

            // These are the bits of the number "444", so the result must be not 444, i.e. 555.
            res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_any_clear(2, 3, 4, 5, 7, 8));
            REQUIRE(res);
            REQUIRE(res->w == 555);

            res = Bar::find_one(MONGO_ODM_KEY(Bar::w).bits_any_clear(3, 5));
            REQUIRE(!res);
        }
    }
}

TEST_CASE("Query builder works with non-ODM class") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    auto point_coll = mongo_odm::odm_collection<Point>(coll);
    point_coll.insert_one({5, 6});
    auto res = point_coll.find_one(MONGO_ODM_KEY(Point::x) == 5);
    REQUIRE(res->x == 5);

    coll.delete_many({});
}

TEST_CASE("Update Builder", "mongo_odm::UpdateExpr") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    Bar::setCollection(coll);
    Bar(444, 1, 2, false, "hello", {0, 0}, {4, 5, 6}, {{1, 2}, {3, 4}}, system_clock::now()).save();
    Bar(444, 1, 3, false, "hello", {0, 1}, {4, 5, 6}, {{5, 6}, {7, 8}}, system_clock::now()).save();
    Bar(555, 10, 2, true, "goodbye", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
        .save();

    SECTION("Test = assignment.", "[mongo_odm::UpdateExpr]") {
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                                   (MONGO_ODM_KEY(Bar::z) = "new_str", MONGO_ODM_KEY(Bar::x1) = 73,
                                    MONGO_ODM_KEY(Bar::x2) = 99, MONGO_ODM_CHILD(Bar, p, x) = 100));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->z == "new_str");
        REQUIRE(bar->x1 == 73);
        REQUIRE(bar->x2.value() == 99);
        REQUIRE(bar->p.x == 100);
    }

    SECTION("Test increment operators.", "[mongo_odm::UpdateExpr]") {
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        int initial_x1 = bar->x1;
        int initial_x2 = bar->x2.value();
        int initial_p_x = bar->p.x;

        {
            // Test postfix ++
            auto res = coll.update_one(
                MONGO_ODM_KEY(Bar::w) == 555,
                (MONGO_ODM_KEY(Bar::x1)++, MONGO_ODM_KEY(Bar::x2)++, MONGO_ODM_CHILD(Bar, p, x)++));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 + 1);
            REQUIRE(bar->x2.value() == initial_x2 + 1);
            REQUIRE(bar->p.x == initial_p_x + 1);
        }

        {
            // Test prefix ++
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            auto res = coll.update_one(
                MONGO_ODM_KEY(Bar::w) == 555,
                (++MONGO_ODM_KEY(Bar::x1), ++MONGO_ODM_KEY(Bar::x2), ++MONGO_ODM_CHILD(Bar, p, x)));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 + 1);
            REQUIRE(bar->x2.value() == initial_x2 + 1);
            REQUIRE(bar->p.x == initial_p_x + 1);
        }

        {
            // Test postfix --
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            auto res = coll.update_one(
                MONGO_ODM_KEY(Bar::w) == 555,
                (MONGO_ODM_KEY(Bar::x1)--, MONGO_ODM_KEY(Bar::x2)--, MONGO_ODM_CHILD(Bar, p, x)--));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 - 1);
            REQUIRE(bar->x2.value() == initial_x2 - 1);
            REQUIRE(bar->p.x == initial_p_x - 1);
        }

        {
            // Test prefix --
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            auto res = coll.update_one(
                MONGO_ODM_KEY(Bar::w) == 555,
                (--MONGO_ODM_KEY(Bar::x1), --MONGO_ODM_KEY(Bar::x2), --MONGO_ODM_CHILD(Bar, p, x)));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 - 1);
            REQUIRE(bar->x2.value() == initial_x2 - 1);
            REQUIRE(bar->p.x == initial_p_x - 1);
        }

        {
            // Test operator+=
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                                       (MONGO_ODM_KEY(Bar::x1) += 5, MONGO_ODM_KEY(Bar::x2) += 5,
                                        MONGO_ODM_CHILD(Bar, p, x) += 5));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 + 5);
            REQUIRE(bar->x2.value() == initial_x2 + 5);
            REQUIRE(bar->p.x == initial_p_x + 5);
        }

        {
            // Test operator-=
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                                       (MONGO_ODM_KEY(Bar::x1) -= 5, MONGO_ODM_KEY(Bar::x2) -= 5,
                                        MONGO_ODM_CHILD(Bar, p, x) -= 5));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 - 5);
            REQUIRE(bar->x2.value() == initial_x2 - 5);
            REQUIRE(bar->p.x == initial_p_x - 5);
        }

        {
            // Test operator*=
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                                       (MONGO_ODM_KEY(Bar::x1) *= 5, MONGO_ODM_KEY(Bar::x2) *= 5,
                                        MONGO_ODM_CHILD(Bar, p, x) *= 5));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 * 5);
            REQUIRE(bar->x2.value() == initial_x2 * 5);
            REQUIRE(bar->p.x == initial_p_x * 5);
        }
    }

    SECTION("Test bitwise update operators.", "[mongo_odm::BitUpdateExpr]") {
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        int initial_x1 = bar->x1;

        // Bitwise OR
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555, MONGO_ODM_KEY(Bar::x1) |= 7);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->x1 == (initial_x1 | 7));

        // Bitwise AND
        initial_x1 = bar->x1;
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555, MONGO_ODM_KEY(Bar::x1) &= 20);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->x1 == (initial_x1 & 20));

        // Bitwise XOR
        initial_x1 = bar->x1;
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555, MONGO_ODM_KEY(Bar::x1) ^= initial_x1);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->x1 == 0);
    }

    SECTION("Test $setOnInsert operator.", "[mongo_odm::Nvp::set_on_insert]") {
        auto opts = mongocxx::options::update{};
        opts.upsert(true);

        // This should match an existing document, and thus not modify Bar::z.
        auto res =
            coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                            MONGO_ODM_KEY(Bar::z).set_on_insert("This was set on insert!"), opts);
        REQUIRE(res);
        REQUIRE(!res->upserted_id());
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->z != "This was set on insert!");

        // This should create a new document, and set Bar::z to the new value.
        coll.delete_many(MONGO_ODM_KEY(Bar::w) == -999);
        res = coll.update_one(
            MONGO_ODM_KEY(Bar::w) == -999,
            (MONGO_ODM_KEY(Bar::x1) = bar->x1, MONGO_ODM_KEY(Bar::x2) = bar->x2.value(),
             MONGO_ODM_KEY(Bar::y) = bar->y,
             MONGO_ODM_KEY(Bar::z).set_on_insert("This was set on insert!"),
             MONGO_ODM_KEY(Bar::p) = bar->p, MONGO_ODM_KEY(Bar::arr) = bar->arr,
             MONGO_ODM_KEY(Bar::pts) = bar->pts, MONGO_ODM_KEY(Bar::t) = bar->t),
            opts);
        REQUIRE(res);
        REQUIRE(res->upserted_id());

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == -999);
        REQUIRE(bar);
        REQUIRE(bar->z == "This was set on insert!");
    }

    SECTION("Test $unset operator.", "[mongo_odm::Nvp::unset]") {
        Bar(666, 10, 999, true, "unset", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 666, MONGO_ODM_KEY(Bar::x2).unset());
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 666);
        REQUIRE(bar);
        REQUIRE(!bar->x2);
    }

    SECTION("Test $min operator.", "[mongo_odm::Nvp::min]") {
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->w == 555);
        int initial_x1 = bar->x1;

        // This update should do nothing, since the new value is larger than the old
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                                   MONGO_ODM_KEY(Bar::x1).min(initial_x1 + 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 0);

        // This update should modify the document, since the new value is smaller than the old
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                              MONGO_ODM_KEY(Bar::x1).min(initial_x1 - 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
    }

    SECTION("Test $max operator.", "[mongo_odm::Nvp::max]") {
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->w == 555);
        int initial_x1 = bar->x1;

        // This update should do nothing, since the new value is smaller than the old
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                                   MONGO_ODM_KEY(Bar::x1).max(initial_x1 - 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 0);

        // This update should modify the document, since the new value is larger than the old
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                              MONGO_ODM_KEY(Bar::x1).max(initial_x1 + 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
    }

    SECTION("Test $currentDate operator.") {
        // initialize document with old date
        hours hrs(1);
        system_clock::time_point old_time = system_clock::now() - hrs;
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555, MONGO_ODM_KEY(Bar::t) = old_time);
        REQUIRE(res);

        // set document with current date
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 555,
                              MONGO_ODM_KEY(Bar::t) = mongo_odm::current_date);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        // Check that document's date is more recent than old date.
        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 555);
        REQUIRE(bar);
        auto time_diff =
            std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now() - bar->t);
        REQUIRE(time_diff < hrs);
    }

    /* Array update operators */

    SECTION("Test $pop array update operator.", "[mongo_odm::Nvp::pop]") {
        Bar(777, 10, 999, true, "pop", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
            .save();
        auto res =
            coll.update_one(MONGO_ODM_KEY(Bar::w) == 777, MONGO_ODM_KEY(Bar::arr).pop(false));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 777);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{5, 6}));

        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 777, MONGO_ODM_KEY(Bar::arr).pop(true));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 777);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{5}));
    }

    SECTION("Test $pull array update operator", "[mongo_odm::Nvp::pull]") {
        Bar(888, 10, 999, true, "pull", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
            .save();

        // use pull() with a value
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 888, MONGO_ODM_KEY(Bar::arr).pull(5));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 888);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 6}));

        // use pull() with a query
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 888,
                              MONGO_ODM_KEY(Bar::pts).pull(MONGO_ODM_KEY(Point::x) > 10));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 888);
        REQUIRE(bar);
        REQUIRE((bar->pts == std::vector<Point>{{9, 10}}));
    }

    SECTION("Test $pullAll array update operator", "[mongo_odm::Nvp::pull]") {
        Bar(1010, 10, 999, true, "pullAll", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();

        auto points = std::vector<int>{5, 6, 7};
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1010,
                                   MONGO_ODM_KEY(Bar::arr).pull_all(points));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1010);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4}));
    }

    SECTION("Test $addToSet array update operator.", "[mongo_odm::Nvp::add_to_set]") {
        Bar(1111, 10, 999, true, "addToSet", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();
        // Add existing element to array, document is unchanged
        auto res =
            coll.update_one(MONGO_ODM_KEY(Bar::w) == 1111, MONGO_ODM_KEY(Bar::arr).add_to_set(4));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 0);

        // Add new element to array, document is modified.
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1111, MONGO_ODM_KEY(Bar::arr).add_to_set(7));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1111);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7}));

        // Add array of elements to array.
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1111,
                              MONGO_ODM_KEY(Bar::arr).add_to_set(std::vector<int>{7, 8, 9}));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1111);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7, 8, 9}));
    }

    SECTION("Test $push array update operator.", "[mongo_odm::Nvp::push]") {
        Bar(1212, 10, 999, true, "push", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();
        auto res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1212, MONGO_ODM_KEY(Bar::arr).push(7));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7}));

        // push several element at once
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1212,
                              MONGO_ODM_KEY(Bar::arr).push(std::vector<int>{7, 8, 9}));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7, 7, 8, 9}));

        // push with $slice option
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1212,
                              MONGO_ODM_KEY(Bar::arr).push(std::vector<int>{}).slice(4));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7}));

        // push with $sort option that is an integer +/-1
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1212,
                              MONGO_ODM_KEY(Bar::arr).push(std::vector<int>{}).sort(-1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{7, 6, 5, 4}));

        // push with $sort options that is an expression {field: +/-1}
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1212,
                              MONGO_ODM_KEY(Bar::pts)
                                  .push(std::vector<Point>{})
                                  .sort(MONGO_ODM_KEY(Point::x).sort(false)));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->pts == std::vector<Point>{{11, 12}, {9, 10}}));

        // push with $position option, to insert new elements at a specific place in the array.
        res = coll.update_one(MONGO_ODM_KEY(Bar::w) == 1212,
                              MONGO_ODM_KEY(Bar::arr).push(std::vector<int>{1, 2, 3}).position(1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MONGO_ODM_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{7, 1, 2, 3, 6, 5, 4}));
    }
}
