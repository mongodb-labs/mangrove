+++
next = "/3-queries/operators"
prev = "/3-queries"
title = "Introduction"
toc = true
weight = 1

+++

Mangrove provides a **query builder** with static type-checking for constructing MongoDB queries.
This allows you to compare documents using familiar C++ syntax using operators such as `>` and `!=`.
These query objects are automatically converted to BSON, and can be passed either to the Mangrove Model's `find` and `find_one` methods, to the Mangrove collection wrapper, or to the query methods in the Mongo C++ Driver.

The query builder helps you avoid JSON syntax errors, such as forgetting a closing curly brace, and will catch other errors at compile time --- so you don't have to wait until your queries return errors in production (or silently return incorrect results!) to catch mistakes.

Mangrove currently supports all the {{% a_blank "query operators" "https://docs.mongodb.com/v3.2/reference/operator/query/" %}} as of MongoDB 3.2,
except for `$type`, `$where`, and geospatial queries. If you need to use these operators, create the BSON for the query as you {{% a_blank "normally would" "https://github.com/mongodb/mongo-cxx-driver/wiki/Handling-BSON-in-the-new-driver" %}} using the MongoDB C++ driver.

Consider the following class that represents a user of some website:
```cpp
class User : public mangrove::model<User> {
public:
    std::string username;
    std::int32_t age;

    MANGROVE_MAKE_KEYS_MODEL(User, MANGROVE_NVP(username), MANGROVE_NVP(age));
};
```

To retrieve the users in the database that are over 21 years of age, one would use the following query:

```cpp
// { age: {$gte: 21} }
auto results = User::find(MANGROVE_KEY(User::age) >= 21);
```

Where `collection` is a `mongocxx::collection` object from the C++ Driver.
The different parts of this code will be explained in more detail below,
but note the `MANGROVE_KEY` macro that is used to refer to individual fields in the document.
Then, Mangrove uses {{% a_blank "operator overloading" "http://en.cppreference.com/w/cpp/language/operators" %}} to allow the field to be compared to `21` with the `>=` operator.
This constructs a query object, which is implicitly cast to a BSON value when passed into `mongocxx::collection::find()`.

{{% notice warning %}}
The query objects should only be used as *temporary objects* --- that is, constructed and passed directly into another function, such as `find()`.
Attempting to store these queries in a variable and use them later may produce undefined results.
This is because the query expressions store their arguments by reference to avoid putting too much data on the stack.
{{% /notice %}}

## Specifying fields

You've already seen one way of referring to fields, the `MANGROVE_KEY` macro.
This macro takes a class member specified as `Base::member`, and creates a "field object" that can be used in queries.

#### Referring to embedded fields

Referring to fields in embedded objects is a bit different.
Let's say we wanted to add an Address class to our User:

```cpp
class Address {
public:
    std::string street;
    std::string city;
    std::string state;
    std::string zip;  

    MANGROVE_MAKE_KEYS(Address, MANGROVE_NVP(street), MANGROVE_NVP(city),
                                MANGROVE_NVP(state), MANGROVE_NVP(zip));
};

class User : public mangrove::model<User> {
public:
    std::string username;
    std::int32_t age;
    Address addr;

    MANGROVE_MAKE_KEYS_MODEL(User, MANGROVE_NVP(username), MANGROVE_NVP(age), MANGROVE_NVP(addr));
};
```

Now, we can query for users using their address using the `MANGROVE_CHILD` macro:

```cpp
// { "addr.zip": "21211" }
auto results = User::find(MANGROVE_CHILD(User, addr, zip) == "21211");
```
The `MANGROVE_CHILD` takes a base class, and then the names of each successive nested field.

{{% notice info %}}
Mangrove can support as many levels of nesting as is allowed in the {{% a_blank "BSON specification" "https://docs.mongodb.com/manual/reference/limits/#Nested-Depth-for-BSON-Documents" %}}.
{{% /notice %}}

Another way to refer to children is to link field objects using the `->*` operator:

```cpp
// { "addr.zip": "21211" }
auto results = User::find(MANGROVE_KEY(User::addr) ->* MANGROVE_KEY(Address::zip) == "21211");
```

#### Referring to array elements

Referring to specific array elements in queries is the same as in any C++ expression ---
one can simply user the square brackets `[]` to specify a certain index.
For instance, the following query finds users whose 10th score in some game is higher than 1000:

```cpp
// { "scores.9": {$gt: 1000} }
// (note that arrays are 0-indexed)
auto results = User::find(MANGROVE_KEY(User::scores)[9] > 1000);
```

## Type checking

Mangrove provides type checking on queries, so that comparing fields and values with mismatched types,
or using array operators on non-array fields, results in compile-time errors.

For instance, the following query will fail at compile time:

```cpp
// { age: {$gte: "21"} }
auto results = User::find(MANGROVE_KEY(User::age) >= "21");
```

In the mongo shell, for instance, such a query would be just fine,
but would have done a lexicographic comparison between the `age` field and the string `"21"`.
This is definitely not what we want, and may only have been discovered later, in production.

Similarly, the query

```cpp
// { age: {$all: [15, 17, 19]} }
auto results = User::find(MANGROVE_KEY(User::age).all(std::vector<int>{15, 17, 19}));
```

does not make sense because `User::age` is not an array, and will cause a syntax error accordingly.

## Combining queries

There are several ways to combine different conditions into one query in Mangrove.
The simplest one is the comma operator --- conditions can be appended one after the other, separated by a comma.
This is the same as having comma-separated fields in a BSON query; it has the effect of a logical AND
on the various conditions.

The following query filters by users' age as well as address:

```cpp
// { age: {$gt: 65}, "addr.state": "NY" }
auto results = User::find((MANGROVE_KEY(User::age) > 65, MANGROVE_CHILD(User, addr, state) == "NY"));
```

{{% notice note %}}
Note that the query expression is inside another set of parentheses.
This is to ensure that the different conditions are combined as one query,
instead of passed to `find()` as different arguments.
{{% /notice %}}

Queries can also be combined using boolean operators:

```cpp
// { $and: [
//          {$or: [{age: {$gt: 65}}, {age: {$lt: 5}}]},
//          {"addr.state": "NY"}
//        ] }
auto results = User::find((MANGROVE_KEY(User::age) > 65 || MANGROVE_KEY(User::age) < 5) && MANGROVE_CHILD(User, addr, state) == "NY");
```

{{% notice tip %}}
Using boolean operators also allows you to use the same field in several conditions.
Above, `User::age` was compared twice.
This is not supported in MongoDB when combining queries with commas.
{{% /notice %}}
