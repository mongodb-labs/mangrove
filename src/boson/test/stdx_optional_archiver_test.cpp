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

#include <boson/bson_archiver.hpp>
#include <boson/stdx/optional.hpp>
#include <bsoncxx/json.hpp>
#include <fstream>

using boson::stdx::optional;
using boson::stdx::make_optional;
using boson::stdx::nullopt;

// Optional values.
struct OptDataA {
    optional<int32_t> x, y;
    optional<double> z;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z));
    }

    bool operator==(const OptDataA& other) const {
        return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
    }
};

struct SingleElementDataA {
    int32_t x;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(x));
    }
};

TEST_CASE(
    "the BSONOutputArchive successfully serializes stdx::optional types only when they contain a "
    "value, and the BSONInputArchive successfully deserializes BSON documents that are missing "
    "stdx::optional elements.") {
    OptDataA a1;
    a1.x = 229;

    SingleElementDataA a2;
    a2.x = 43;

    {
        std::ofstream os("single_optional_test.bson");
        boson::BSONOutputArchive oarchive(os);
        oarchive(a1);
    }

    {
        std::ofstream os("single_optional_element_in_class.bson");
        boson::BSONOutputArchive oarchive(os);
        oarchive(a2);
    }

    {
        SingleElementDataA a;
        std::ifstream is("single_optional_test.bson");
        boson::BSONInputArchive iarchive(is);
        iarchive(a);
        REQUIRE(a.x == a1.x);
    }

    {
        OptDataA a;
        a.x = 10;
        a.y = 10;
        a.z = 10.;
        std::ifstream is("single_optional_element_in_class.bson");
        boson::BSONInputArchive iarchive(is);
        iarchive(a);
        REQUIRE(!a.y);
        REQUIRE(!a.z);
        REQUIRE(a.x);
        REQUIRE(*(a.x) == a2.x);
    }
}

// Mixed optional and non-optional values.
struct OptDataB {
    optional<int32_t> x;
    int32_t y;
    optional<double> z;
    double a;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(a));
    }
};

// Optional embedded documents.
struct OptDataC {
    optional<OptDataB> db;
    optional<SingleElementDataA> da;
    int32_t y;
    optional<double> z;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(db), CEREAL_NVP(da), CEREAL_NVP(y), CEREAL_NVP(z));
    }
};

TEST_CASE(
    "the BSON archives successfully serialize and deserialize documents with optional embedded "
    "documents.") {
    OptDataB b1;
    b1.y = 229;
    b1.z = 3.14;
    b1.a = 3.4;

    OptDataC c1;

    c1.db = b1;

    c1.y = 43;

    {
        std::ofstream os("optional_embedded_doc_test.bson");
        boson::BSONOutputArchive oarchive(os);
        oarchive(c1);
    }

    {
        OptDataC c2;
        std::ifstream is("optional_embedded_doc_test.bson");
        boson::BSONInputArchive iarchive(is);
        iarchive(c2);

        REQUIRE(c2.db);
        REQUIRE(c2.db->y == 229);
        REQUIRE(c2.db->z == 3.14);
        REQUIRE(c2.db->a == 3.4);
        REQUIRE(!c2.db->x);
        REQUIRE(!c2.z);
        REQUIRE(c2.y == 43);
        REQUIRE(!c2.da);
    }
}

// Optional embedded arrays.
struct OptDataD {
    optional<std::vector<OptDataA>> v1;
    optional<std::vector<SingleElementDataA>> v2;
    optional<std::vector<int32_t>> v3;

    int32_t y;
    optional<double> z;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(v1), CEREAL_NVP(v2), CEREAL_NVP(v3), CEREAL_NVP(y), CEREAL_NVP(z));
    }
};

TEST_CASE(
    "the BSON archives successfully serialize and deserialize documents with optional embedded "
    "arrays.") {
    OptDataA a1, a2, a3;
    a1.x = 229;
    a2.y = 43;
    a3.z = 1.2345;

    std::vector<OptDataA> avec;
    avec.push_back(a1);
    avec.push_back(a2);
    avec.push_back(a3);

    std::vector<int32_t> intvec;
    intvec.push_back(1);
    intvec.push_back(2);

    OptDataD d;

    d.v1 = avec;
    d.v3 = intvec;

    d.y = 10011;

    {
        std::ofstream os("optional_embedded_array_test.bson");
        boson::BSONOutputArchive oarchive(os);
        oarchive(d);
    }

    {
        OptDataD d2;
        std::ifstream is("optional_embedded_array_test.bson");
        boson::BSONInputArchive iarchive(is);
        iarchive(d2);

        REQUIRE(d2.v1);
        REQUIRE(!d2.v2);
        REQUIRE(d2.v3);
        REQUIRE(d2.y == d.y);
        REQUIRE(!d2.z);

        REQUIRE(std::equal(avec.begin(), avec.end(), d2.v1->begin()));
        REQUIRE(std::equal(intvec.begin(), intvec.end(), d2.v3->begin()));
    }
}

bsoncxx::document::value someNonEmptyScope =
    bsoncxx::from_json(R"({"obj": {"a":1, "b":2, "c":3}})");

struct OptDataE : public boson::UnderlyingBSONDataBase {
    optional<bsoncxx::types::b_double> test_double;
    optional<bsoncxx::types::b_utf8> test_utf8;
    optional<bsoncxx::types::b_date> test_date;
    optional<bsoncxx::types::b_regex> test_regex;
    optional<bsoncxx::types::b_code> test_code;
    optional<bsoncxx::types::b_codewscope> test_codewscope;
    optional<bsoncxx::types::b_symbol> test_symbol;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(test_double), CEREAL_NVP(test_utf8), CEREAL_NVP(test_date),
           CEREAL_NVP(test_regex), CEREAL_NVP(test_code), CEREAL_NVP(test_codewscope),
           CEREAL_NVP(test_symbol));
    }

    OptDataE() = default;
};

TEST_CASE(
    "The BSON archives successfully serialize and deserialize optional bsoncxx::types::b_ types.") {
    OptDataE e1;

    e1.test_utf8 = make_optional<bsoncxx::types::b_utf8>(bsoncxx::types::b_utf8("hello!"));
    e1.test_date = make_optional<bsoncxx::types::b_date>(
        bsoncxx::types::b_date(std::chrono::system_clock::time_point{}));
    e1.test_regex = make_optional<bsoncxx::types::b_regex>(bsoncxx::types::b_regex("a", "b"));
    e1.test_code = make_optional<bsoncxx::types::b_code>(bsoncxx::types::b_code("void main() {}"));
    e1.test_codewscope = make_optional<bsoncxx::types::b_codewscope>(
        bsoncxx::types::b_codewscope("void main() {}", someNonEmptyScope.view()));
    e1.test_symbol = make_optional<bsoncxx::types::b_symbol>(bsoncxx::types::b_symbol("bye."));

    {
        std::ofstream os("optional_b_test.bson");
        boson::BSONOutputArchive oarchive(os);
        oarchive(e1);
    }

    {
        OptDataE e2;
        std::ifstream is("optional_b_test.bson");
        boson::BSONInputArchive iarchive(is);
        iarchive(e2);

        REQUIRE(!e2.test_double);
        REQUIRE(e2.test_utf8->value.to_string() == std::string{"hello!"});
        REQUIRE(e2.test_date->operator std::chrono::system_clock::time_point() ==
                std::chrono::system_clock::time_point{});
        REQUIRE(e2.test_regex->regex.to_string() == std::string("a"));
        REQUIRE(e2.test_regex->options.to_string() == std::string("b"));

        REQUIRE(e2.test_code->code.to_string() == std::string("void main() {}"));
        REQUIRE(e2.test_codewscope->code.to_string() == std::string("void main() {}"));
        REQUIRE(e2.test_symbol->symbol.to_string() == std::string("bye."));
    }
}

struct OptDataF {
    optional<bsoncxx::types::b_utf8> test_utf8;

    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(test_utf8));
    }
};

TEST_CASE(
    "the BSON archives successfully enforce the requirement that classes inherit "
    "UnderlyingBSONDataBase if they contain a b_ view value present in a stdx::optional.") {
    {
        OptDataF f;
        f.test_utf8 = make_optional<bsoncxx::types::b_utf8>(bsoncxx::types::b_utf8("hello there!"));
        std::ofstream os("optional_underlyingbsonbase_test.bson");
        boson::BSONOutputArchive oarchive(os);
        REQUIRE_THROWS(oarchive(f));
    }
}
