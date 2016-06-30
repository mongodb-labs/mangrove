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

#include <mongo_odm/odm_collection.hpp>
#include <mongo_odm/query_builder.hpp>

using namespace mongocxx;
using namespace mongo_odm;

class Bar {
   public:
    Bar *v;
    int64_t w;
    int x1;
    int x2;
    bool y;
    std::string z;
    // this characer is not put into a macro, there will be no member funcion
    // pointer for it.
    char missing;

    ADAPT(Bar, NVP(v), NVP(w), NVP(x1), NVP(x2), NVP(y), NVP(z))

    template <class Archive>
    void serialize(Archive &ar) {
        ar(CEREAL_NVP(w), CEREAL_NVP(x1), CEREAL_NVP(x2), CEREAL_NVP(y), CEREAL_NVP(z));
    }
};
ADAPT_STORAGE(Bar);

TEST_CASE("Query Builder", "[mongo_odm::query_builder]") {
    std::cout << "Wrap: " << wrap(&Bar::v)->name << std::endl;
    std::cout << "Wrap: " << SAFEWRAP(Bar, w).name << std::endl;
    std::cout << "Wrap: " << (SAFEWRAPTYPE(Bar::x1) == 5) << std::endl;
    std::cout << "Wrap: " << (SAFEWRAPTYPE(Bar::x2) == 5) << std::endl;
    std::cout << "Wrap: " << (SAFEWRAPTYPE(Bar::y) == true) << std::endl;
    // Type error:
    // std::cout << "Wrap: " << (SAFEWRAPTYPE(Bar::y) == "hello") << std::endl;
    std::cout << "Wrap: " << (SAFEWRAPTYPE(Bar::z) == "hello") << std::endl;
    // "Missing" Field:
    // std::cout << "Wrap: " << SAFEWRAPTYPE(Bar::missing)->name << std::endl;

    instance::current();
    client conn{uri{}};
    collection coll = conn["testdb"]["testcollection"];
    odm_collection<Bar> bar_coll(coll);

    Bar b{nullptr, 444, 1, 2, false, "hello", 'q'};
    bar_coll.insert_one(b);
}
