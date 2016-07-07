![Mangrove Logo](mangrove_logo.png)
# Mangrove
Mangrove is an object document mapper for MongoDB written in C++.

## Features
Mangrove provides:
- Automatic serialization of complex objects to and from BSON
- A `collection` interface that uses objects instead of BSON
- A static query builder with compile-time checking

## Quickstart

### Install the C++ Driver
Mangrove requires that the MongoDB C++ Driver be installed.
Installation instructions for the driver can be found in the
[Quickstart Guide](https://github.com/mongodb/mongo-cxx-driver/wiki/Quickstart-Guide-(New-Driver)).

### Build and install Mangrove

 * Clone the repository
    - `git clone -b master https://github.com/mongodb/mongo-cxx-odm`

 * Build the driver. Note that if you installed the C++ driver to a path that is automatically searched by `pkgconfig`, you can omit the `PKG_CONFIG_PATH` environment variable. If you don't have `pkgconfig`, you can explicitly set the path to the libbson, libmongoc, bsoncxx, and mongocxx install prefixes with the `-DLIBBSON_DIR`, `-DLIBMONGOC_DIR`, `-DLIBBSONCXX_DIR`, and `-DLIBMONGOCXX_DIR` CMake arguments.
   - `cd mongo-cxx-odm/build`
   - `[PKG_CONFIG_PATH=CDRIVER_INSTALL_PATH/lib/pkgconfig] cmake -DCMAKE_BUILD_TYPE=Release [-DCMAKE_INSTALL_PREFIX=CPPDRIVER-INSTALL-PATH] ..`
   - `make && sudo make install`

### Example
Fire up your editor and copy in this code to a file called `hellomangrove.cpp`:

```cpp
#include <iostream>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

#include <mongo_odm/model.hpp>
#include <mongo_odm/query_builder.hpp>

class Manatee : public mongo_odm::model<Manatee> {
public:
    int age;
    std::string name;
    double weight;

    ODM_MAKE_KEYS_MODEL(NVP(age), NVP(name), NVP(weight));
};

int main(int, char**) {
    // connect to database
    mongocxx::instance inst{};
    mongocxx::client conn{mongocxx::uri{}};
    auto collection = conn["testdb"]["testcollection"];
    collection.delete_many({});

    Manatee::setCollection(collection);
    Manatee m1 = Manatee{12, "Melvin", 650.1};
    Manatee m2 = Manatee{11, "Marvin", 600.4};
    m1.save();
    m2.save();

    auto cursor = Manatee::find(ODM_KEY(Manatee::age) > 10 && ODM_KEY(Manatee::weight) <= 700);
    for (Manatee m_tmp : cursor){
        std::cout << "Manatee: " << m_tmp.name << ", " << m_tmp.age << "years old." << std::endl;
    }

    m1.remove();
    m2.remove();

    // num_manatees should be 0 since we've removed m1 and m2 from the database.
    size_t num_manatees = collection.count({});
    std::cout << num_manates << std::endl;
}
```

## Documentation
...

## Bugs and Issues
See our [JIRA Project](https://jira.mongodb.org/browse/CXXODM/).

## License
The source files in this repository are made available under the terms of the Apache License, version 2.0.
