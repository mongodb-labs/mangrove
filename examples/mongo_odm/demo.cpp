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

#include <chrono>
#include <string>
#include <vector>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pipeline.hpp>

#include <bson_mapper/bson_streambuf.hpp>
#include <cereal/types/vector.hpp>
#include <mongo_odm/odm_collection.hpp>

// Makes dealing with dates easier
typedef std::chrono::system_clock sysclock_t;

class Grade {
   public:
    std::chrono::system_clock::time_point date;
    std::string grade;
    int32_t score;

    // A serialization function that passes in the fields to be serialized.
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(date), CEREAL_NVP(grade), CEREAL_NVP(score));
    }
};

std::ostream& operator<<(std::ostream& os, const Grade& g) {
    std::time_t t = std::chrono::system_clock::to_time_t(g.date);
    os << "(" << g.grade << ", " << g.score << ", " << std::ctime(&t) << ")";
    return os;
}

class Address {
   public:
    std::string building;
    std::string street;
    std::string zipcode;
    double lat;
    double lng;

    // A serialization function that passes in the fields to be serialized.
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(building), CEREAL_NVP(street), CEREAL_NVP(zipcode), CEREAL_NVP(lat),
           CEREAL_NVP(lng));
    }
};

std::ostream& operator<<(std::ostream& os, const Address& a) {
    os << "[" << a.building << " " << a.street << ", " << a.zipcode << " "
       << "(" << a.lat << ", " << a.lng << ")]";
    return os;
}

class Restaurant {
   public:
    Address address;
    std::string borough;
    std::string cuisine;
    std::vector<Grade> grades;
    std::string name;
    std::string restaurant_id;

    // A serialization function that passes in the fields to be serialized.
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(address), CEREAL_NVP(borough), CEREAL_NVP(cuisine), CEREAL_NVP(grades),
           CEREAL_NVP(name), CEREAL_NVP(restaurant_id));
    }
};

/**
 * A class to store aggregate statistics. In this case, it keeps track of a borough and how many
 * restaurants are in it.
 */
class BoroughStats {
   public:
    std::string borough;
    int count;

    // A serialization function that passes in the fields to be serialized.
    template <class Archive>
    void serialize(Archive& ar) {
        ar(CEREAL_NVP(borough), CEREAL_NVP(count));
    }
};

sysclock_t::time_point make_time_point(long seconds) {
    std::chrono::duration<double, std::ratio<1>> since_epoch_full(seconds);
    auto since_epoch = std::chrono::duration_cast<sysclock_t::duration>(since_epoch_full);
    return sysclock_t::time_point(since_epoch);
}

int main(int, char**) {
    mongocxx::instance inst{};
    mongocxx::client conn{mongocxx::uri{}};
    auto db = conn["test"];
    auto restaurants_col = db["restaurants"];
    restaurants_col.delete_many({});
    // wrap mongocxx::collection in an odm_collection
    mongo_odm::odm_collection<Restaurant> restaurants(restaurants_col);

    // Create sample restaurant object

    Restaurant r1;
    r1.address = {"1480", "2 Avenue", "10075", -73.9557413, 40.7720266};
    r1.borough = "Manhattan";
    r1.cuisine = "Italian";
    r1.grades.push_back({sysclock_t::now(), "A", 11});
    r1.grades.push_back({sysclock_t::now(), "B", 17});
    r1.name = "Vella";
    r1.restaurant_id = "41704620";

    // Create sample restaurant object
    Restaurant r2;
    r2.address = {"558", "7 Avenue", "10018", -73.984472, 40.759011};
    r2.borough = "Manhattan";
    r2.cuisine = "Middle Eastern";
    r2.grades.push_back({sysclock_t::now(), "B", 11});
    r2.grades.push_back({sysclock_t::now(), "C", 17});
    r2.name = "Maoz";
    r2.restaurant_id = "41704621";

    // insert restaurants into collection
    auto res = restaurants.insert_one(r1);
    res = restaurants.insert_one(r2);

    // get all restaurants in collection
    {
        std::cout << "Finding all restaurants in the collection..." << std::endl;
        mongo_odm::deserializing_cursor<Restaurant> cur = restaurants.find({});
        for (const auto& r : cur) {
            std::cout << "Restaurant: " << r.name << std::endl;
        }
        std::cout << std::endl;
    }

    // Query by top-level field
    {
        std::cout << "Querying by cuisine (top-level field):" << std::endl;
        bsoncxx::builder::stream::document filter;
        filter << "cuisine"
               << "Italian";
        auto r_italian = restaurants.find_one(filter.view());
        if (r_italian) {
            std::cout << "Restaurant: " << r_italian->name << " (" << r_italian->cuisine << ")"
                      << std::endl;
        } else {
            std::cout << "No matching restaurants were found." << std::endl;
        }
        std::cout << std::endl;
    }

    // Query by embedded field
    {
        std::cout << "Querying by street (embedded field):" << std::endl;
        bsoncxx::builder::stream::document filter;
        filter << "address.street"
               << "7 Avenue";
        auto cur = restaurants.find(filter.view());
        for (const auto& r : cur) {
            std::cout << "Restaurant: " << r.name << " " << r.address << std::endl;
        }
        std::cout << std::endl;
    }

    // Pass options to find()
    {
        std::cout << "Pass options to find(): " << std::endl;
        mongocxx::options::find opts;
        bsoncxx::builder::stream::document order_builder;
        order_builder << "borough" << -1 << "address.zipcode" << 1;
        opts.sort(order_builder.view());

        auto cur = restaurants.find({}, opts);
        for (const auto& r : cur) {
            std::cout << "Restaurant: " << r.name << ", " << r.borough << " " << r.address.zipcode
                      << std::endl;
        }
        std::cout << std::endl;
    }

    // Insert several objects at once
    {
        std::cout << "Insert multiple objects using a container:" << std::endl;
        Restaurant r3;
        r3.name = "Morris Park Bake Shop";
        r3.cuisine = "Bakery";
        r3.borough = "Bronx";
        r3.address = {"1007", "Morris Park Ave", "10462", -73.856077, 40.848447};
        r3.grades.push_back({make_time_point(1393804800000), "A", 2});
        r3.grades.push_back({make_time_point(1378857600000), "A", 6});
        r3.grades.push_back({make_time_point(1358985600000), "A", 10});
        r3.grades.push_back({make_time_point(1322006400000), "A", 9});
        r3.grades.push_back({make_time_point(1299715200000), "B", 14});
        r3.restaurant_id = "30075445";

        Restaurant r4;
        r4.name = "Wendy's";
        r4.cuisine = "Hamburgers";
        r4.borough = "Brooklyn";
        r4.address = {"469", "Flatbush Avenue", "11225", -73.961704, 40.662942};
        r4.grades.push_back({make_time_point(1419897600000), "A", 8});
        r4.grades.push_back({make_time_point(1404172800000), "B", 23});
        r4.grades.push_back({make_time_point(1367280000000), "A", 12});
        r4.grades.push_back({make_time_point(1336435200000), "A", 12});
        r4.restaurant_id = "30112340";

        std::vector<Restaurant> restaurant_vec;
        restaurant_vec.push_back(r3);
        restaurant_vec.push_back(r4);
        auto res = restaurants.insert_many(restaurant_vec);
        int c = res->inserted_count();
        std::cout << "Inserted " << c << " restaurants, from vector of size "
                  << restaurant_vec.size() << "." << std::endl;
        std::cout << std::endl;
    }

    // Replace documents
    {
        std::cout << "Replace documents using object parameters:" << std::endl;
        bsoncxx::builder::stream::document filter;
        filter << "name"
               << "Vella";
        // update address of restaurant object
        r1.address = {"47", "W. 13th St.", "10011", -73.961101, 40.662333};
        std::cout << "Restaurant: " << r1.name << ", new address: " << r1.address << std::endl;
        auto res = restaurants.find_one_and_replace(filter.view(), r1);
        std::cout << "Returned document, restaurant: " << res->name
                  << ", old address: " << res->address << std::endl;
        std::cout << std::endl;
    }

    // Aggregation example
    {
        using bsoncxx::builder::stream::open_document;
        using bsoncxx::builder::stream::close_document;

        std::cout << "Store aggregation results in custom objects:" << std::endl;

        // Build an aggregation query that find the number of restaurants in each borough
        mongocxx::pipeline stages;
        bsoncxx::builder::stream::document group_stage;
        group_stage << "_id"
                    << "$borough"
                    << "count" << open_document << "$sum" << 1 << close_document;
        bsoncxx::builder::stream::document project_stage;
        project_stage << "borough"
                      << "$_id"
                      << "count"
                      << "$count";
        stages.group(group_stage.view());
        stages.project(project_stage.view());

        auto cur = restaurants.aggregate<BoroughStats>(stages);
        for (const auto& bs : cur) {
            std::cout << "Borough: " << bs.borough << ", restaurants: " << bs.count << std::endl;
        }
        std::cout << std::endl;
    }
    return 0;
}
