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

#include <typeinfo>

#include <mongo_odm/util.hpp>

using bsoncxx::stdx::optional;

TEST_CASE("select_non_void properly returns non-void type", "[mongo_odm::select_non_void]") {
    REQUIRE((typeid(mongo_odm::select_non_void<void, int>::type)) == typeid(int));
    REQUIRE((typeid(mongo_odm::select_non_void<int, double>::type)) == typeid(int));
}

TEST_CASE("Test all_true template struct.", "[mongo_odm::all_true]") {
    REQUIRE((mongo_odm::all_true<false>::value) == false);
    REQUIRE((mongo_odm::all_true<false, true>::value) == false);
    REQUIRE((mongo_odm::all_true<true, false>::value) == false);
    REQUIRE((mongo_odm::all_true<true, false, true>::value) == false);
    REQUIRE((mongo_odm::all_true<true, true>::value) == true);
    REQUIRE((mongo_odm::all_true<true, true, true>::value) == true);
}

TEST_CASE(
    "is_arithmetic_optional should contain true only for types that are optionals containing "
    "arithmetic types.",
    "[mongo_odm::is_arithmetic_optional]") {
    REQUIRE((mongo_odm::is_arithmetic_optional<int>::value) == false);
    REQUIRE((mongo_odm::is_arithmetic_optional<char *>::value) == false);
    REQUIRE((mongo_odm::is_arithmetic_optional<optional<char *>>::value) == false);
    REQUIRE((mongo_odm::is_arithmetic_optional<optional<int>>::value) == true);
    REQUIRE((mongo_odm::is_arithmetic_optional<optional<double>>::value) == true);
}
