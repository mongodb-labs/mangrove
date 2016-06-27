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

#include <mongo_odm/query_builder.hpp>

using namespace mongo_odm;

struct Foo {
  Foo *v;
  long w;
  int x;
  bool y;
  std::string z;
  // this characer is not put into a macro, there will be no member funcion
  // pointer for it.
  char missing;

  ADAPT(Foo, NVP(v), NVP(w), NVP(x), NVP(y), NVP(z))
};
ADAPT_STORAGE(Foo);

TEST_CASE("Query Builder", "[mongo_odm::query_builder]") {
  std::cout << "Wrap: " << wrap(&Foo::v)->name << std::endl;
  std::cout << "Wrap: " << SAFEWRAP(Foo, w).name << std::endl;
  std::cout << "Wrap: " << SAFEWRAPTYPE(Foo::x).name << std::endl;
  std::cout << "Wrap: " << (SAFEWRAPTYPE(Foo::y) == true) << std::endl;
  // Type error:
  // std::cout << "Wrap: " << (SAFEWRAPTYPE(Foo::y) == "hello") << std::endl;
  std::cout << "Wrap: " << (SAFEWRAPTYPE(Foo::z) == "hello") << std::endl;
  // "Missing" Field:
  // std::cout << "Wrap: " << SAFEWRAPTYPE(Foo::missing)->name << std::endl;
}
