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

#include <mangrove/collection_wrapper.hpp>
#include <mangrove/model.hpp>
#include <mangrove/query_builder.hpp>

#include <bsoncxx/types.hpp>

using namespace std::chrono;
using namespace mongocxx;

// An ODM class that does not rely on model
class Point {
   public:
    int x;
    int y;
    MANGROVE_MAKE_KEYS(Point, MANGROVE_NVP(x), MANGROVE_NVP(y));

    bool operator==(const Point& rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }
};

// An ODM class that inherits from model
class Bar : public mangrove::model<Bar> {
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

    MANGROVE_MAKE_KEYS_MODEL(Bar, MANGROVE_NVP(w), MANGROVE_NVP(x1), MANGROVE_NVP(x2),
                             MANGROVE_NVP(y), MANGROVE_NVP(z), MANGROVE_NVP(p), MANGROVE_NVP(arr),
                             MANGROVE_NVP(pts), MANGROVE_NVP(t));

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

    BarParent() = default;

    // default constructor
    BarParent(Bar b) : b(b) {
    }
    MANGROVE_MAKE_KEYS(BarParent, MANGROVE_NVP(b));
};

TEST_CASE("Test member access.", "[mangrove::nvp]") {
    SECTION("Member access using MANGROVE_KEY") {
        auto nvp = MANGROVE_KEY(Bar::x1);
        REQUIRE(nvp.get_name() == "x1");
        REQUIRE(nvp.t == &Bar::x1);
    }

    SECTION("Member access using MANGROVE_CHILD macro") {
        auto nvp = MANGROVE_CHILD(Bar, p);
        REQUIRE(nvp.get_name() == "p");
        REQUIRE(nvp.t == &Bar::p);
    }
}

class OptionalWithChildren {
   public:
    stdx::optional<Point> pt;
    stdx::optional<std::vector<Point>> pts_vec;

    OptionalWithChildren() = default;

    // default constructor
    OptionalWithChildren(Point pt, stdx::optional<std::vector<Point>> pts_vec)
        : pt(pt), pts_vec(pts_vec) {
    }

    MANGROVE_MAKE_KEYS(OptionalWithChildren, MANGROVE_NVP(pt), MANGROVE_NVP(pts_vec));
};

TEST_CASE("Test nested member access.", "[mangrove::nvp_child]") {
    SECTION("Nested member access using operator->*") {
        // nvp's must be used as temporary objects.
        auto test_lambda = [](const auto& nvp_child) {
            REQUIRE(nvp_child.get_name() == "p.x");
            REQUIRE(nvp_child.t == &Point::x);
            REQUIRE(nvp_child.parent.get_name() == "p");
            REQUIRE(nvp_child.parent.t == &Bar::p);
        };
        test_lambda(MANGROVE_KEY(Bar::p)->*MANGROVE_KEY(Point::x));
    }

    SECTION("Multiple nested member access using operator->*") {
        // Nvp's must be used as temporary objects.
        auto test_lambda = [](const auto& nvp_child) {
            REQUIRE(nvp_child.get_name() == "b.p.x");
            REQUIRE(nvp_child.t == &Point::x);
            REQUIRE(nvp_child.parent.get_name() == "b.p");
            REQUIRE(nvp_child.parent.t == &Bar::p);
            REQUIRE(nvp_child.parent.parent.get_name() == "b");
            REQUIRE(nvp_child.parent.parent.t == &BarParent::b);
        };
        test_lambda(MANGROVE_KEY(BarParent::b)->*MANGROVE_KEY(Bar::p)->*MANGROVE_KEY(Point::x));
    }

    SECTION("Nested member access using MANGROVE_CHILD macro") {
        // Nvp's must be used as temporary objects.
        auto test_lambda = [](const auto& nvp_child) {
            REQUIRE(nvp_child.get_name() == "p.x");
            REQUIRE(nvp_child.t == &Point::x);
            REQUIRE(nvp_child.parent.get_name() == "p");
            REQUIRE(nvp_child.parent.t == &Bar::p);
        };
        test_lambda(MANGROVE_CHILD(Bar, p, x));
    }

    SECTION("Multiple nested member access using MANGROVE_CHILD macro") {
        // Nvp's must be used as temporary objects.
        auto test_lambda = [](const auto& nvp_child) {
            REQUIRE(nvp_child.get_name() == "b.p.x");
            REQUIRE(nvp_child.t == &Point::x);
            REQUIRE(nvp_child.parent.get_name() == "b.p");
            REQUIRE(nvp_child.parent.t == &Bar::p);
            REQUIRE(nvp_child.parent.parent.get_name() == "b");
            REQUIRE(nvp_child.parent.parent.t == &BarParent::b);
        };
        test_lambda(MANGROVE_CHILD(BarParent, b, p, x));
    }

    SECTION("Test accessing nested members within optional") {
        REQUIRE((MANGROVE_KEY(OptionalWithChildren::pt)->*MANGROVE_KEY(Point::x)).get_name() ==
                "pt.x");
        REQUIRE((MANGROVE_KEY(OptionalWithChildren::pt)->*MANGROVE_KEY(Point::x)).t == &Point::x);
        REQUIRE(MANGROVE_CHILD(OptionalWithChildren, pt, x).get_name() == "pt.x");
        REQUIRE(MANGROVE_CHILD(OptionalWithChildren, pt, x).t == &Point::x);
    }

    SECTION("Test accessing nested members within arrays") {
        // test array element acces
        REQUIRE((MANGROVE_KEY(Bar::pts)->*MANGROVE_KEY(Point::x)).get_name() == "pts.x");
        REQUIRE((MANGROVE_KEY(Bar::pts)->*MANGROVE_KEY(Point::x)).t == &Point::x);

        REQUIRE((MANGROVE_CHILD(Bar, pts, x).get_name() == "pts.x"));
        REQUIRE((MANGROVE_CHILD(Bar, pts, x).t == &Point::x));

        // test optional array element access
        REQUIRE((MANGROVE_KEY(OptionalWithChildren::pts_vec)->*MANGROVE_KEY(Point::x)).get_name() ==
                "pts_vec.x");
        REQUIRE((MANGROVE_KEY(OptionalWithChildren::pts_vec)->*MANGROVE_KEY(Point::x)).t ==
                &Point::x);

        REQUIRE((MANGROVE_CHILD(OptionalWithChildren, pts_vec, x).get_name() == "pts_vec.x"));
        REQUIRE((MANGROVE_CHILD(OptionalWithChildren, pts_vec, x).t == &Point::x));
    }
}

TEST_CASE("Test *.element() and *_ELEM for referring to scalar array elements",
          "mangrove::free_nvp") {
    // test MANGROVE_KEY(...).element()
    REQUIRE((std::is_same<typename decltype(MANGROVE_KEY(Bar::arr).element())::type, int>::value));
    // test MANGROVE_ELEM(...)
    REQUIRE((std::is_same<typename decltype(MANGROVE_ELEM(Bar::arr))::type, int>::value));
    // test MANGROVE_CHILD(...).element()
    REQUIRE((std::is_same<typename decltype(MANGROVE_CHILD(BarParent, b, arr).element())::type,
                          int>::value));
    // test MANGROVE_CHILD_ELEM(...)
    REQUIRE((
        std::is_same<typename decltype(MANGROVE_CHILD_ELEM(BarParent, b, arr))::type, int>::value));
}

/**
 * Helper class with a custom field name.
 * Used for testing the MANGROVE_CUSTOM_NVP macro.
 */
class CustomNameField {
   public:
    int x;

    CustomNameField() {
    }

    // default constructor
    CustomNameField(int x) : x(x) {
    }

    MANGROVE_MAKE_KEYS(CustomNameField, MANGROVE_CUSTOM_NVP(x, "custom"));
};

TEST_CASE("Test keys with custom names") {
    instance::current();
    client conn{uri{}};
    auto coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    auto res = coll.insert_one(boson::to_document(CustomNameField(4)));
    REQUIRE(res);
    auto query = bsoncxx::from_json(R"({"custom": 4})");
    auto doc = coll.find_one(query.view());
    REQUIRE(doc);
    REQUIRE(doc->view()["custom"].get_value().get_int32().value == 4);
    auto obj = boson::to_obj<CustomNameField>(doc.value());
    REQUIRE(obj.x == 4);
}

TEST_CASE("Test member array access") {
    // Nvp's must be used as temporary objects.
    REQUIRE((MANGROVE_KEY(Bar::arr)[1].get_name() == "arr.1"));
    REQUIRE((std::is_same<typename decltype(MANGROVE_KEY(Bar::arr)[1])::no_opt_type, int>::value ==
             true));

    REQUIRE(((MANGROVE_KEY(Bar::pts)[1]->*MANGROVE_KEY(Point::x)).get_name() == "pts.1.x"));
    REQUIRE((std::is_same<typename decltype(
                              MANGROVE_KEY(Bar::pts)[1]->*MANGROVE_KEY(Point::x))::no_opt_type,
                          int>::value == true));
}

TEST_CASE("Query Builder", "[mangrove::query_builder]") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];

    Bar::setCollection(coll);
    Bar::delete_many({});
    Bar(444, 1, 2, false, "hello", {0, 0}, {4, 5, 6}, {{1, 2}, {3, 4}}, system_clock::now()).save();
    Bar(444, 1, 3, false, "hello", {0, 1}, {4, 5, 6}, {{5, 6}, {7, 8}}, system_clock::now()).save();
    Bar(555, 10, 2, true, "goodbye", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
        .save();

    SECTION("Test == comparison.", "[mangrove::comparison_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) == 1);
        REQUIRE(res);
        REQUIRE(res->x1 == 1);

        // Test optional type
        res = Bar::find_one(MANGROVE_KEY(Bar::x2) == 3);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        res = Bar::find_one(MANGROVE_KEY(Bar::z) == "hello");
        REQUIRE(res);
        REQUIRE(res->z == "hello");

        // nested member test
        res = Bar::find_one(MANGROVE_KEY(Bar::p)->*MANGROVE_KEY(Point::x) == 1);
        REQUIRE(res);
        REQUIRE(res->p.x == 1);

        // Complex type test
        res = Bar::find_one(MANGROVE_KEY(Bar::p) == Point{0, 1});
        REQUIRE(res);
        REQUIRE((res->p == Point{0, 1}));

        // scalar array (vector) member test
        res = Bar::find_one(MANGROVE_KEY(Bar::arr) == std::vector<int>{4, 5, 6});
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(MANGROVE_KEY(Bar::pts) == std::vector<Point>{{1, 2}, {3, 4}});
        REQUIRE(res);
        REQUIRE((res->pts == std::vector<Point>{{1, 2}, {3, 4}}));

        // Array element test
        res = Bar::find_one(MANGROVE_KEY(Bar::arr)[2] == 6);
        REQUIRE(res);
        REQUIRE(res->arr[2] == 6);
    }

    SECTION("Test > comparison.", "[mangrove::comparison_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) > 1);
        REQUIRE(res);
        REQUIRE(res->x1 > 1);

        // Test optional type
        res = Bar::find_one(MANGROVE_KEY(Bar::x2) > 2);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        // Test lexicographical comparison
        res = Bar::find_one(MANGROVE_KEY(Bar::z) > "goodbye");
        REQUIRE(res);
        REQUIRE(res->z == "hello");

        // nested member test
        res = Bar::find_one(MANGROVE_KEY(Bar::p)->*MANGROVE_KEY(Point::x) > 0);
        REQUIRE(res);
        REQUIRE(res->p.x > 0);

        // Complex type test, lexicographical comparison
        res = Bar::find_one(MANGROVE_KEY(Bar::p) > Point{0, 1});
        REQUIRE(res);
        REQUIRE((res->p == Point{1, 0}));

        // scalar array (vector) member test (lexicographical comparison)
        res = Bar::find_one(MANGROVE_KEY(Bar::arr) > std::vector<int>{1, 2, 3});
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(MANGROVE_KEY(Bar::pts) > std::vector<Point>{{5, 6}, {7, 8}});
        REQUIRE(res);
        REQUIRE((res->pts == std::vector<Point>{{9, 10}, {11, 12}}));

        // Array element test
        res = Bar::find_one(MANGROVE_KEY(Bar::arr)[2] > 5);
        REQUIRE(res);
        REQUIRE(res->arr[2] == 6);
    }

    SECTION("Test >= comparison.", "[mangrove::comparison_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) >= 10);
        REQUIRE(res);
        REQUIRE(res->x1 >= 10);

        // Test optional type
        res = Bar::find_one(MANGROVE_KEY(Bar::x2) >= 3);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        // Test lexicographical comparison
        res = Bar::find_one(MANGROVE_KEY(Bar::z) >= "hello");
        REQUIRE(res);
        REQUIRE(res->z == "hello");
    }

    SECTION("Test < comparison.", "[mangrove::comparison_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) < 10);
        REQUIRE(res);
        REQUIRE(res->x1 < 10);

        // Test optional type
        res = Bar::find_one(MANGROVE_KEY(Bar::x2) < 3);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 2);

        // Test lexicographical comparison
        res = Bar::find_one(MANGROVE_KEY(Bar::z) < "hello");
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test <= comparison.", "[mangrove::comparison_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) <= 1);
        REQUIRE(res);
        REQUIRE(res->x1 <= 1);

        // Test optional type
        res = Bar::find_one(MANGROVE_KEY(Bar::x2) <= 2);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 2);

        // Test lexicographical comparison
        res = Bar::find_one(MANGROVE_KEY(Bar::z) <= "goodbye");
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test != comparison.", "[mangrove::comparison_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) != 1);
        REQUIRE(res);
        REQUIRE(res->x1 != 1);

        // Test optional type
        res = Bar::find_one(MANGROVE_KEY(Bar::x2) != 2);
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        res = Bar::find_one(MANGROVE_KEY(Bar::z) != "hello");
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // Complex type test
        res = Bar::find_one(!(MANGROVE_KEY(Bar::p) < Point{1, 0}));
        REQUIRE(res);
        REQUIRE((res->p == Point{1, 0}));

        // nested member test
        res = Bar::find_one(MANGROVE_KEY(Bar::p)->*MANGROVE_KEY(Point::x) != 0);
        REQUIRE(res);
        REQUIRE(res->p.x != 0);

        // Array element test
        res = Bar::find_one(MANGROVE_KEY(Bar::arr)[2] != 5);
        REQUIRE(res);
        REQUIRE(res->arr[2] == 6);
    }

    SECTION("Test $not expression, with operator!.", "[mangrove::not_expr]") {
        auto res = Bar::find_one(!(MANGROVE_KEY(Bar::x1) < 10));
        REQUIRE(res);
        REQUIRE(res->x1 >= 10);

        res = Bar::find_one(!(MANGROVE_KEY(Bar::z) == "hello"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // Test optional type
        res = Bar::find_one(!(MANGROVE_KEY(Bar::x2) <= 2));
        REQUIRE(res);
        REQUIRE(res->x2.value() == 3);

        // nested member test
        res = Bar::find_one(!(MANGROVE_KEY(Bar::p)->*MANGROVE_KEY(Point::x) == 1));
        REQUIRE(res);
        REQUIRE(res->p.x != 1);

        // array (vector) member test
        res = Bar::find_one(!(MANGROVE_KEY(Bar::arr) == std::vector<int>{1, 5, 6}));
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(!(MANGROVE_KEY(Bar::pts) == std::vector<Point>{{1, 2}, {3, 4}}));
        REQUIRE(res);
        REQUIRE((res->pts != std::vector<Point>{{1, 2}, {3, 4}}));

        // Array element test
        res = Bar::find_one(!(MANGROVE_KEY(Bar::arr)[2] < 5));
        REQUIRE(res);
        REQUIRE(res->arr[2] == 6);
    }

    SECTION("Test $nin and $in operators.", "[mangrove::in_array_expr]") {
        std::vector<int> nums;
        nums.push_back(1);
        nums.push_back(2);
        nums.push_back(555);

        auto res = Bar::find_one(MANGROVE_KEY(Bar::w).in(nums));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        res = Bar::find_one(MANGROVE_KEY(Bar::x2).in(nums));
        REQUIRE(res);
        REQUIRE(res->x2 == 2);

        nums.clear();
        nums.push_back(1);
        nums.push_back(2);
        nums.push_back(444);

        res = Bar::find_one(MANGROVE_KEY(Bar::w).nin(nums));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        res = Bar::find_one(MANGROVE_KEY(Bar::x2).nin(nums));
        REQUIRE(res);
        REQUIRE(res->x2 == 3);

        res = Bar::find_one(MANGROVE_CHILD(Bar, p, y).nin(nums) && MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(res);
        REQUIRE(res->w == 555);
        REQUIRE(res->p.y == 0);

        // Test query builder with arrays of complex values
        res = Bar::find_one(MANGROVE_KEY(Bar::p).in(std::vector<Point>{{0, 1}, {0, 2}}));
        REQUIRE(res);
        REQUIRE((res->p == Point{0, 1}));

        // scalar array (vector) member test
        res = Bar::find_one(MANGROVE_KEY(Bar::arr).in(std::vector<int>{1, 2, 6}));
        REQUIRE(res);
        REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

        // document array (vector) member test
        res = Bar::find_one(MANGROVE_KEY(Bar::pts).in(std::vector<Point>{{1, 2}, {3, 4}}));
        REQUIRE(res);
        REQUIRE((res->pts == std::vector<Point>{{1, 2}, {3, 4}}));
    }

    SECTION("Test expression list.", "[mangrove::expression_list]") {
        auto res = Bar::find_one((MANGROVE_KEY(Bar::x1) == 1, MANGROVE_KEY(Bar::x2) == 2,
                                  MANGROVE_KEY(Bar::w) >= 444, MANGROVE_CHILD(Bar, p, y) == 0));
        REQUIRE(res);
        REQUIRE(res->x1 == 1);
        REQUIRE(res->x2 == 2);
        REQUIRE(res->w >= 444);
        REQUIRE(res->p.x == 0);
    }

    SECTION("Test boolean expressions.", "[mangrove::boolean_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x1) > 9 && MANGROVE_KEY(Bar::x1) < 11 &&
                                 MANGROVE_CHILD(Bar, p, x) > 0);
        REQUIRE(res);
        REQUIRE(res->x1 == 10);
        REQUIRE(res->x1 == 10);

        auto cursor = Bar::find(MANGROVE_KEY(Bar::x1) == 10 || MANGROVE_KEY(Bar::x2) == 3 ||
                                MANGROVE_CHILD(Bar, p, x) == 1);
        int i = 0;
        for (Bar b : cursor) {
            i++;
            // Check that x1 is equal to 10 or x2 is equal to 3
            bool or_test = (b.x1 == 10) || (b.x2 == 3);
            REQUIRE(or_test);
        }
        REQUIRE(i == 2);

        // Test a complex boolean expression, with parentheses and mixed oeprators
        res =
            Bar::find_one((MANGROVE_KEY(Bar::z) == "goodbye" || !(MANGROVE_KEY(Bar::y) == false)) &&
                          (MANGROVE_KEY(Bar::w) == 555 || MANGROVE_KEY(Bar::x2) == 3));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test boolean operators on expression lists. (used primarily for $nor)",
            "[mangrove::boolean_list_expr]") {
        auto res =
            Bar::find_one(nor(MANGROVE_KEY(Bar::w) == 444, MANGROVE_KEY(Bar::x1) == 1,
                              MANGROVE_KEY(Bar::x2) == 3, MANGROVE_KEY(Bar::y) == false,
                              MANGROVE_KEY(Bar::z) == "hello", MANGROVE_CHILD(Bar, p, x) == 0));
        REQUIRE(res);
        Bar b = res.value();
        REQUIRE(b.w != 444);
        REQUIRE(b.x1 != 1);
        REQUIRE(b.x2 != 3);
        REQUIRE(b.y != false);
        REQUIRE(b.z != "hello");
        REQUIRE(b.p.x != 0);
    }

    SECTION("Test $exists operator on optional fields.", "[mangrove::nvp::exists]") {
        auto count = Bar::count(MANGROVE_KEY(Bar::x2).exists(true));
        REQUIRE(count == 3);

        Bar(444, 1, stdx::nullopt, false, "hello", {0, 0}, {4, 5, 6}, {{1, 2}}, system_clock::now())
            .save();
        auto res = Bar::find_one(MANGROVE_KEY(Bar::x2).exists(false));
        REQUIRE(res);
        REQUIRE(!res->x2);
    }

    SECTION("Test $mod operator.", "[mangrove::mod_expr]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::w).mod(50, 5));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        // Not Expressions can contain Mod Expressions
        res = Bar::find_one(!MANGROVE_KEY(Bar::w).mod(50, 5));
        REQUIRE(res);
        REQUIRE(res->w == 444);

        res = Bar::find_one(!!MANGROVE_KEY(Bar::w).mod(50, 5));
        REQUIRE(res);
        REQUIRE(res->w == 555);

        // Test with array element
        res = Bar::find_one(MANGROVE_KEY(Bar::arr)[2].mod(5, 1));
        REQUIRE(res);
        REQUIRE(res->arr[2] == 6);
    }

    SECTION("Test $text operator.", "[mangrove::text_search_expr]") {
        auto value = bsoncxx::from_json(R"({"z": "text"})");
        coll.create_index(value.view());

        auto res = Bar::find_one(mangrove::text("goodbye"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        res = Bar::find_one(mangrove::text("goodbye").language("en"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // case sensitivity is off by default.
        res = Bar::find_one(mangrove::text("GOODBYE").language("en"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // test with case sensitivity on
        res = Bar::find_one(mangrove::text("GOODBYE").language("en").case_sensitive(true));
        REQUIRE(!res);

        // diactric sensitivity is off by default.
        res = Bar::find_one(mangrove::text(u8"goódbye").language("en"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // test with diactric sensitivity on
        res = Bar::find_one(mangrove::text(u8"goódbye").language("en").diacritic_sensitive(true));
        REQUIRE(!res);
    }

    SECTION("Test $regex operator.", "[mangrove::nvp::regex]") {
        auto res = Bar::find_one(MANGROVE_KEY(Bar::z).regex("o+d", ""));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        res = Bar::find_one(MANGROVE_KEY(Bar::z).regex("O+D", "i"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");

        // Test $not with regex.
        res = Bar::find_one(!MANGROVE_KEY(Bar::z).regex("O+D", "i"));
        REQUIRE(res);
        REQUIRE(res->z == "hello");

        res = Bar::find_one(!!MANGROVE_KEY(Bar::z).regex("O+D", "i"));
        REQUIRE(res);
        REQUIRE(res->z == "goodbye");
    }

    SECTION("Test array query operators") {
        SECTION("Test $size operator.", "[mangrove::nvp::size]") {
            auto res = Bar::find_one(MANGROVE_KEY(Bar::arr).size(3));
            REQUIRE(res);
            REQUIRE(res->arr.size() == 3);
        }

        SECTION("Test $all operator.", "[mangrove::nvp::all]") {
            auto res = Bar::find_one(MANGROVE_KEY(Bar::arr).all(std::vector<int>{4, 5, 6}));
            REQUIRE(res);
            REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

            res = Bar::find_one(MANGROVE_KEY(Bar::arr).all(std::vector<int>{4, 5, 6, 7}));
            REQUIRE(!res);
        }

        SECTION("Test $elem_match operator.", "[mangrove::nvp::elem_match]") {
            // Test scalar array
            auto res = Bar::find_one(MANGROVE_KEY(Bar::arr).elem_match(
                (MANGROVE_ELEM(Bar::arr) > 5, MANGROVE_ELEM(Bar::arr) < 7)));
            REQUIRE(res);
            REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

            // Test scalar array with $not
            res = Bar::find_one(!MANGROVE_KEY(Bar::arr).elem_match(MANGROVE_ELEM(Bar::arr) > 6));
            REQUIRE(res);
            REQUIRE((res->arr == std::vector<int>{4, 5, 6}));

            // Test document array
            res = Bar::find_one(MANGROVE_KEY(Bar::pts).elem_match(
                (MANGROVE_KEY(Point::x) > 7, MANGROVE_KEY(Point::y) > 8)));
            REQUIRE(res);
            REQUIRE((res->pts == std::vector<Point>{{9, 10}, {11, 12}}));

            // Test document array with $not
            res = Bar::find_one(!MANGROVE_KEY(Bar::pts).elem_match(
                (MANGROVE_KEY(Point::x) <= 7, MANGROVE_KEY(Point::y) <= 8)));
            REQUIRE(res);
            REQUIRE((res->pts == std::vector<Point>{{9, 10}, {11, 12}}));
        }
    }

    SECTION("Test bitwise query operators") {
        SECTION("Test $bitsAllSet operators.", "[mangrove::nvp::bits_all_set]") {
            auto res = Bar::find_one(MANGROVE_KEY(Bar::x1).bits_all_set(10));
            REQUIRE(res);
            REQUIRE(res->x1 == 10);

            res = Bar::find_one(MANGROVE_KEY(Bar::x1).bits_all_set(1, 3));
            REQUIRE(res);
            REQUIRE(res->x1 == 10);

            res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_all_set(1, 2, 3, 4));
            REQUIRE(!res);
        }

        SECTION("Test $bitsAnySet operators.", "[mangrove::nvp::bits_any_set]") {
            auto res = Bar::find_one(MANGROVE_KEY(Bar::x1).bits_any_set(10));
            REQUIRE(res);
            REQUIRE((res->x1 | 10) > 0);

            res = Bar::find_one(MANGROVE_KEY(Bar::x1).bits_any_set(1, 3));
            REQUIRE(res);
            REQUIRE((res->x1 | (1 << 1) | (1 << 3)) > 0);

            res = Bar::find_one(MANGROVE_KEY(Bar::x1).bits_any_set(12, 13));
            REQUIRE(!res);
        }

        SECTION("Test $bitsAllClear operators.", "[mangrove::nvp::bits_all_clear]") {
            auto res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_all_clear((~555) & 0xFFFF));
            REQUIRE(res);
            REQUIRE(res->w == 555);

            res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_all_clear(2, 4, 6, 7, 8));
            REQUIRE(res);
            REQUIRE(res->w == 555);

            res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_all_clear(0, 1, 2, 3, 4, 5, 6));
            REQUIRE(!res);
        }

        SECTION("Test $bitsAnyClear operators.", "[mangrove::nvp::bits_any_clear]") {
            auto res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_any_clear(21));
            REQUIRE(res);
            REQUIRE((res->w & 21) < 21);

            // These are the bits of the number "444", so the result must be not 444, i.e. 555.
            res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_any_clear(2, 3, 4, 5, 7, 8));
            REQUIRE(res);
            REQUIRE(res->w == 555);

            res = Bar::find_one(MANGROVE_KEY(Bar::w).bits_any_clear(3, 5));
            REQUIRE(!res);
        }
    }
}

TEST_CASE("Query builder works with non-ODM class") {
    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    coll.delete_many({});

    auto point_coll = mangrove::collection_wrapper<Point>(coll);
    point_coll.insert_one({5, 6});
    auto res = point_coll.find_one(MANGROVE_KEY(Point::x) == 5);
    REQUIRE(res->x == 5);

    coll.delete_many({});
}

TEST_CASE("Update Builder", "mangrove::update_expr") {
    instance::current();
    client conn{uri{}};

    Bar::setCollection(conn["testdb"]["testcollection"]);
    Bar::delete_many({});
    Bar(444, 1, 2, false, "hello", {0, 0}, {4, 5, 6}, {{1, 2}, {3, 4}}, system_clock::now()).save();
    Bar(444, 1, 3, false, "hello", {0, 1}, {4, 5, 6}, {{5, 6}, {7, 8}}, system_clock::now()).save();
    Bar(555, 10, 2, true, "goodbye", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
        .save();

    SECTION("Test = assignment.", "[mangrove::update_expr]") {
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                   (MANGROVE_KEY(Bar::z) = "new_str", MANGROVE_KEY(Bar::x1) = 73,
                                    MANGROVE_KEY(Bar::x2) = 99, MANGROVE_CHILD(Bar, p, x) = 100,
                                    MANGROVE_KEY(Bar::arr)[2] = 600));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->z == "new_str");
        REQUIRE(bar->x1 == 73);
        REQUIRE(bar->x2.value() == 99);
        REQUIRE(bar->p.x == 100);
        REQUIRE(bar->arr[2] == 600);
    }

    SECTION("Test increment operators.", "[mangrove::update_expr]") {
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        int initial_x1 = bar->x1;
        int initial_x2 = bar->x2.value();
        int initial_p_x = bar->p.x;
        int initial_arr_2 = bar->arr[2];

        {
            // Test postfix ++
            auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                       (MANGROVE_KEY(Bar::x1)++, MANGROVE_KEY(Bar::x2)++,
                                        MANGROVE_CHILD(Bar, p, x)++, MANGROVE_KEY(Bar::arr)[2]++));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 + 1);
            REQUIRE(bar->x2.value() == initial_x2 + 1);
            REQUIRE(bar->p.x == initial_p_x + 1);
            REQUIRE(bar->arr[2] == initial_arr_2 + 1);
        }

        {
            // Test prefix ++
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            initial_arr_2 = bar->arr[2];
            auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                       (++MANGROVE_KEY(Bar::x1), ++MANGROVE_KEY(Bar::x2),
                                        ++MANGROVE_CHILD(Bar, p, x), ++MANGROVE_KEY(Bar::arr)[2]));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 + 1);
            REQUIRE(bar->x2.value() == initial_x2 + 1);
            REQUIRE(bar->p.x == initial_p_x + 1);
            REQUIRE(bar->arr[2] == initial_arr_2 + 1);
        }

        {
            // Test postfix --
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            initial_arr_2 = bar->arr[2];
            auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                       (MANGROVE_KEY(Bar::x1)--, MANGROVE_KEY(Bar::x2)--,
                                        MANGROVE_CHILD(Bar, p, x)--, MANGROVE_KEY(Bar::arr)[2]--));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 - 1);
            REQUIRE(bar->x2.value() == initial_x2 - 1);
            REQUIRE(bar->p.x == initial_p_x - 1);
            REQUIRE(bar->arr[2] == initial_arr_2 - 1);
        }

        {
            // Test prefix --
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            initial_arr_2 = bar->arr[2];
            auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                       (--MANGROVE_KEY(Bar::x1), --MANGROVE_KEY(Bar::x2),
                                        --MANGROVE_CHILD(Bar, p, x), --MANGROVE_KEY(Bar::arr)[2]));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 - 1);
            REQUIRE(bar->x2.value() == initial_x2 - 1);
            REQUIRE(bar->p.x == initial_p_x - 1);
            REQUIRE(bar->arr[2] == initial_arr_2 - 1);
        }

        {
            // Test operator+=
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            initial_arr_2 = bar->arr[2];
            auto res =
                Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                (MANGROVE_KEY(Bar::x1) += 5, MANGROVE_KEY(Bar::x2) += 5,
                                 MANGROVE_CHILD(Bar, p, x) += 5, MANGROVE_KEY(Bar::arr)[2] += 5));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 + 5);
            REQUIRE(bar->x2.value() == initial_x2 + 5);
            REQUIRE(bar->p.x == initial_p_x + 5);
            REQUIRE(bar->arr[2] == initial_arr_2 + 5);
        }

        {
            // Test operator-=
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            initial_arr_2 = bar->arr[2];
            auto res =
                Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                (MANGROVE_KEY(Bar::x1) -= 5, MANGROVE_KEY(Bar::x2) -= 5,
                                 MANGROVE_CHILD(Bar, p, x) -= 5, MANGROVE_KEY(Bar::arr)[2] -= 5));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 - 5);
            REQUIRE(bar->x2.value() == initial_x2 - 5);
            REQUIRE(bar->p.x == initial_p_x - 5);
            REQUIRE(bar->arr[2] == initial_arr_2 - 5);
        }

        {
            // Test operator*=
            initial_x1 = bar->x1;
            initial_x2 = bar->x2.value();
            initial_p_x = bar->p.x;
            initial_arr_2 = bar->arr[2];
            auto res =
                Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                                (MANGROVE_KEY(Bar::x1) *= 5, MANGROVE_KEY(Bar::x2) *= 5,
                                 MANGROVE_CHILD(Bar, p, x) *= 5, MANGROVE_KEY(Bar::arr)[2] *= 5));
            REQUIRE(res);
            REQUIRE(res->modified_count() == 1);
            bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
            REQUIRE(bar);
            REQUIRE(bar->x1 == initial_x1 * 5);
            REQUIRE(bar->x2.value() == initial_x2 * 5);
            REQUIRE(bar->p.x == initial_p_x * 5);
            REQUIRE(bar->arr[2] == initial_arr_2 * 5);
        }
    }

    SECTION("Test bitwise update operators.", "[mangrove::bit_update_expr]") {
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        int initial_x1 = bar->x1;

        // Bitwise OR
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1) |= 7);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->x1 == (initial_x1 | 7));

        // Bitwise AND
        initial_x1 = bar->x1;
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1) &= 20);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->x1 == (initial_x1 & 20));

        // Bitwise XOR
        initial_x1 = bar->x1;
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1) ^= initial_x1);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->x1 == 0);
    }

    SECTION("Test $setOnInsert operator.", "[mangrove::nvp::set_on_insert]") {
        auto opts = mongocxx::options::update{};
        opts.upsert(true);

        // This should match an existing document, and thus not modify Bar::z.
        auto res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                            MANGROVE_KEY(Bar::z).set_on_insert("This was set on insert!"), opts);
        REQUIRE(res);
        REQUIRE(!res->upserted_id());
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->z != "This was set on insert!");

        // This should create a new document, and set Bar::z to the new value.
        Bar::delete_many(MANGROVE_KEY(Bar::w) == -999);
        res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == -999,
                            (MANGROVE_KEY(Bar::x1) = bar->x1,
                             MANGROVE_KEY(Bar::x2) = bar->x2.value(), MANGROVE_KEY(Bar::y) = bar->y,
                             MANGROVE_KEY(Bar::z).set_on_insert("This was set on insert!"),
                             MANGROVE_KEY(Bar::p) = bar->p, MANGROVE_KEY(Bar::arr) = bar->arr,
                             MANGROVE_KEY(Bar::pts) = bar->pts, MANGROVE_KEY(Bar::t) = bar->t),
                            opts);
        REQUIRE(res);
        REQUIRE(res->upserted_id());

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == -999);
        REQUIRE(bar);
        REQUIRE(bar->z == "This was set on insert!");
    }

    SECTION("Test $unset operator.", "[mangrove::nvp::unset]") {
        Bar(666, 10, 999, true, "unset", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 666, MANGROVE_KEY(Bar::x2).unset());
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 666);
        REQUIRE(bar);
        REQUIRE(!bar->x2);
    }

    SECTION("Test $min operator.", "[mangrove::nvp::min]") {
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->w == 555);
        int initial_x1 = bar->x1;

        // This update should do nothing, since the new value is larger than the old
        auto res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1).min(initial_x1 + 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 0);

        // This update should modify the document, since the new value is smaller than the old
        res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1).min(initial_x1 - 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
    }

    SECTION("Test $max operator.", "[mangrove::nvp::max]") {
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->w == 555);
        int initial_x1 = bar->x1;

        // This update should do nothing, since the new value is smaller than the old
        auto res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1).max(initial_x1 - 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 0);

        // This update should modify the document, since the new value is larger than the old
        res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::x1).max(initial_x1 + 1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);
    }

    SECTION("Test $currentDate operator.") {
        // initialize document with old date
        hours hrs(1);
        system_clock::time_point old_time = system_clock::now() - hrs;
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555, MANGROVE_KEY(Bar::t) = old_time);
        REQUIRE(res);

        // set document with current date
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 555,
                              MANGROVE_KEY(Bar::t) = mangrove::current_date);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        // Check that document's date is more recent than old date.
        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        auto time_diff =
            std::chrono::duration_cast<std::chrono::milliseconds>(system_clock::now() - bar->t);
        REQUIRE(time_diff < hrs);
    }

    /* Array update operators */

    SECTION("Test $ array update operator.", "[mangrove::dollar_operator_nvp]") {
        auto res =
            Bar::update_one((MANGROVE_KEY(Bar::w) == 555,
                             MANGROVE_KEY(Bar::arr).elem_match(MANGROVE_ELEM(Bar::arr) == 5)),
                            MANGROVE_KEY(Bar::arr).first_match() = 500);
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 555);
        REQUIRE(bar);
        REQUIRE(bar->arr[1] == 500);
    }

    SECTION("Test $pop array update operator.", "[mangrove::nvp::pop]") {
        Bar(777, 10, 999, true, "pop", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
            .save();
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 777, MANGROVE_KEY(Bar::arr).pop(false));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 777);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{5, 6}));

        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 777, MANGROVE_KEY(Bar::arr).pop(true));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 777);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{5}));
    }

    SECTION("Test $pull array update operator", "[mangrove::nvp::pull]") {
        Bar(888, 10, 999, true, "pull", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}}, system_clock::now())
            .save();

        // use pull() with a value
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 888, MANGROVE_KEY(Bar::arr).pull(5));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 888);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 6}));

        // use pull() with a query
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 888,
                              MANGROVE_KEY(Bar::pts).pull(MANGROVE_KEY(Point::x) > 10));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 888);
        REQUIRE(bar);
        REQUIRE((bar->pts == std::vector<Point>{{9, 10}}));
    }

    SECTION("Test $pullAll array update operator", "[mangrove::nvp::pull]") {
        Bar(1010, 10, 999, true, "pullAll", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();

        auto points = std::vector<int>{5, 6, 7};
        auto res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 1010, MANGROVE_KEY(Bar::arr).pull_all(points));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1010);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4}));
    }

    SECTION("Test $addToSet array update operator.", "[mangrove::nvp::add_to_set]") {
        Bar(1111, 10, 999, true, "addToSet", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();
        // Add existing element to array, document is unchanged
        auto res =
            Bar::update_one(MANGROVE_KEY(Bar::w) == 1111, MANGROVE_KEY(Bar::arr).add_to_set(4));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 0);

        // Add new element to array, document is modified.
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1111, MANGROVE_KEY(Bar::arr).add_to_set(7));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1111);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7}));

        // Add array of elements to array.
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1111,
                              MANGROVE_KEY(Bar::arr).add_to_set(std::vector<int>{7, 8, 9}));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1111);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7, 8, 9}));
    }

    SECTION("Test $push array update operator.", "[mangrove::nvp::push]") {
        Bar(1212, 10, 999, true, "push", {1, 0}, {4, 5, 6}, {{9, 10}, {11, 12}},
            system_clock::now())
            .save();
        auto res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1212, MANGROVE_KEY(Bar::arr).push(7));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        auto bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7}));

        // push several element at once
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1212,
                              MANGROVE_KEY(Bar::arr).push(std::vector<int>{7, 8, 9}));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7, 7, 8, 9}));

        // push with $slice option
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1212,
                              MANGROVE_KEY(Bar::arr).push(std::vector<int>{}).slice(4));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{4, 5, 6, 7}));

        // push with $sort option that is an integer +/-1
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1212,
                              MANGROVE_KEY(Bar::arr).push(std::vector<int>{}).sort(-1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{7, 6, 5, 4}));

        // push with $sort options that is an expression {field: +/-1}
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1212,
                              MANGROVE_KEY(Bar::pts)
                                  .push(std::vector<Point>{})
                                  .sort(MANGROVE_KEY(Point::x).sort(false)));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->pts == std::vector<Point>{{11, 12}, {9, 10}}));

        // push with $position option, to insert new elements at a specific place in the array.
        res = Bar::update_one(MANGROVE_KEY(Bar::w) == 1212,
                              MANGROVE_KEY(Bar::arr).push(std::vector<int>{1, 2, 3}).position(1));
        REQUIRE(res);
        REQUIRE(res->modified_count() == 1);

        bar = Bar::find_one(MANGROVE_KEY(Bar::w) == 1212);
        REQUIRE(bar);
        REQUIRE((bar->arr == std::vector<int>{7, 1, 2, 3, 6, 5, 4}));
    }
}
