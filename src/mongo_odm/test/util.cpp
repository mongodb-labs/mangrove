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

#include <deque>
#include <forward_list>
#include <list>
#include <queue>
#include <set>
#include <unordered_set>
#include <valarray>

#include <mongo_odm/util.hpp>

using namespace mongo_odm;

using bsoncxx::stdx::optional;

TEST_CASE("all_true struct contains true only if all boolean template parameters are true.",
          "[all_true]") {
    REQUIRE((all_true<false>::value) == false);
    REQUIRE((all_true<true>::value) == true);
    REQUIRE((all_true<false, true>::value) == false);
    REQUIRE((all_true<true, false>::value) == false);
    REQUIRE((all_true<true, false, true>::value) == false);
    REQUIRE((all_true<true, true>::value) == true);
    REQUIRE((all_true<true, true, true>::value) == true);
}

TEST_CASE(
    "is_string contains true only if the template type parameter is an std::basic string, or some "
    "form of C string.") {
    REQUIRE(is_string<int>::value == false);
    REQUIRE(is_string<const int>::value == false);
    REQUIRE(is_string<char const *>::value == true);
    REQUIRE(is_string<wchar_t const *>::value == true);
    REQUIRE(is_string<char *>::value == true);
    REQUIRE(is_string<wchar_t *>::value == true);
    REQUIRE(is_string<char[5]>::value == true);
    REQUIRE(is_string<wchar_t[5]>::value == true);
    REQUIRE(is_string<char const[5]>::value == true);
    REQUIRE(is_string<wchar_t const[5]>::value == true);
    REQUIRE(is_string<char(&)[5]>::value == true);
    REQUIRE(is_string<wchar_t(&)[5]>::value == true);
    REQUIRE(is_string<char const(&)[5]>::value == true);
    REQUIRE(is_string<wchar_t const(&)[5]>::value == true);
    REQUIRE(is_string<std::string>::value == true);
    REQUIRE(is_string<std::basic_string<wchar_t>>::value == true);
}

TEST_CASE(
    "is_iterable contains true only if the template type parameter is an iterable container. This "
    "includes C arrays and std strings.",
    "[is_iterable]") {
    CHECK(is_iterable<int>::value == false);
    CHECK(is_iterable<const int *>::value == false);
    // C arrays are iterable (i.e. can be passed by reference into std::begin() and std::end())
    CHECK(is_iterable<int[5]>::value == true);
    // NOTE: std::string's are iterable
    CHECK(is_iterable<std::string>::value == true);
    CHECK(is_iterable_not_string<std::string>::value == false);
    // Check that the container types supported by the BSON Archiver are iterable
    CHECK(is_iterable<std::vector<int>>::value == true);
    CHECK(is_iterable<std::set<int>>::value == true);
    CHECK(is_iterable<std::forward_list<int>>::value == true);
    CHECK(is_iterable<std::list<int>>::value == true);
    CHECK(is_iterable<std::deque<int>>::value == true);
    CHECK(is_iterable<std::unordered_set<int>>::value == true);
    CHECK(is_iterable<std::valarray<int>>::value == true);
}

TEST_CASE(
    "iterable_value contains the value type of an iterable container, or the given type if it is "
    "not a container. ") {
    CHECK((std::is_same<iterable_value<int>, int>::value));
    CHECK((std::is_same<iterable_value<std::string>, char>::value));
    CHECK((std::is_same<iterable_value<std::vector<int>>, int>::value));
    CHECK((std::is_same<iterable_value<std::vector<std::string>>, std::string>::value));
    // iterable_value only unwraps one level of container.
    CHECK((std::is_same<iterable_value<std::vector<std::vector<int>>>, std::vector<int>>::value));
}

TEST_CASE("is_optional struct contains true only if template type parameter is an optional",
          "[is_optional]") {
    REQUIRE(is_optional<int>::value == false);
    REQUIRE(is_optional<optional<int>>::value == true);
}

TEST_CASE(
    "remove_optional, when templated with an optional type, contains the underlying wrapped type",
    "[remove_optional]") {
    REQUIRE((std::is_same<remove_optional<int>::type, int>::value));
    REQUIRE((std::is_same<remove_optional<std::string>::type, std::string>::value));
    REQUIRE((std::is_same<remove_optional<optional<int>>::type, int>::value));
    REQUIRE((std::is_same<remove_optional<optional<std::string>>::type, std::string>::value));
}

TEST_CASE(
    "bit_positions_to_mask converts a variadic list of bit positions to a mask in which bits at "
    "those positions are 1, and elsewhere are 0.",
    "bit_positions_to_mask") {
    REQUIRE(bit_positions_to_mask() == 0);
    REQUIRE(bit_positions_to_mask(0) == 1);
    REQUIRE(bit_positions_to_mask(1) == 2);
    REQUIRE(bit_positions_to_mask(1, 3) == 10);
    REQUIRE(bit_positions_to_mask(1, 3, 4) == 26);
    REQUIRE(bit_positions_to_mask(1, 1, 3, 3, 4, 4) == 26);
}
