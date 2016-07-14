+++
next = "/2-models/custom_id"
prev = "/2-models/introduction"
title = "Allowed Types"
toc = true
weight = 2

+++

As we discussed in the previous section, there are a limited number of types that you can save and load, or **serialize** in the database within your models. Here we'll enumerate every type that you can include in the `MANGROVE_MAKE_KEYS_MODEL` macro.

## Primitive Types

Mangrove officially supports the following four primitive types:

* `int32_t`
* `int64_t`
* `double`
* `bool`

{{% notice warning %}}
Mangrove will also accept other primitive types such as `int`, `long`, or `unsigned int`, but we highly recommend that you go with one of the four supported types. Other types will be implicitly cast to one of the four supported types, which might cause unpredictable behavior that differs from platform to platform.
{{% /notice %}}

## Strings

Mangrove supports the serialization of `std::string`. If you prefer to use string "views" that minimize the number of copies your program makes, check out the `bsoncxx::types::b_utf8` type discussed in [BSON "View" Types](/2-models/allowed-types/#bson-view-types).

## Time Points

Mangrove supports `std::chrono::system_clock::time_point`. This type is preferred for representing a point in time. If your program uses the C-style `std::time_t` type, we recommend that you use the `to_time_t()` and `from_time_t()` functions discussed in {{% a_blank "the documentation for `std::chrono::system_clock`" "http://en.cppreference.com/w/cpp/chrono/system_clock" %}} so that your class can support `time_point`.

## Containers

Mangrove supports a number of C++ STL Containers as serializable. The following containers will work as long as the type they contain is one of the allowed types discussed on this page.

* `std::vector`
* `std::set`
* `std::unordered_set`
* `std::list`
* `std::forward_list`
* `std::deque`
* `std::valarray`

These are all serialized in the database as BSON arrays.

## Embedded Documents

MongoDB documents often contain embedded documents, which most likely don't logically map to a common C++ type. Fortunately, Mangrove allows you to create classes that represent subdocuments that may be present in a normal document.

Creating a serializable subclass is straightfoward, and very similar to creating a model. Simply take your class, and instead of inheriting from `mangrove::model` and calling `MANGROVE_MAKE_KEYS_MODEL`, just call `MANGROVE_MAKE_KEYS` on the types you want serialized in the subdocument.

```cpp
class ContactCard {
    std::string description;
    std::string address;
    std::string phone;

    MANGROVE_MAKE_KEYS(ContactCard,
                       MANGROVE_NVP(description),
                       MANGROVE_NVP(address),
                       MANGROVE_NVP(phone))
}
```

With the above class definition, you can now have a model like this:

```cpp
class Employee : public mmangrove::model<Employee> {
    std::string name;
    double hourly_wage;
    ContactCard contact_info;

    MANGROVE_MAKE_KEYS_MODEL(Employee,
                             MANGROVE_NVP(name),
                             MANGROVE_NVP(hourly_wage),
                             MANGROVE_NVP(contact_info))
}
```

You can even include subdocuments in an STL container like this:

```cpp
class Employee : public mmangrove::model<Employee> {
    std::string name;
    double hourly_wage;
    std::vector<ContactCard> contact_info;

    MANGROVE_MAKE_KEYS_MODEL(Employee,
                             MANGROVE_NVP(name),
                             MANGROVE_NVP(hourly_wage),
                             MANGROVE_NVP(contact_info))
}
```

And the resulting document would look something like this:

```json
{
    "_id" : ObjectId("578fcf796119b016e0a3b079"),
    "name" : "Jenny",
    "hourly_wage" : 19.81,
    "contact_info" : [
        {
            "type" : "home",
            "phone" : "212-867-5309",
            "address" : "247 E 26 St, New York, NY"
        },
        {
            "type" : "work",
            "phone" : "555-123-4567",
            "address" : "570 Second Ave, New York, NY"
        }
    ]
}
```

The only restriction regarding serializable subdocuments is that they must be *default constructible*.

## Optionals

Although {{% a_blank "`std::optional<T>`" "http://en.cppreference.com/w/cpp/utility/optional/optional" %}} won't be available until C++17, Mangrove offers it as a serializable type in the form of `bson_mapper::stdx::optional<T>`. `optional` is a powerful new C++ type that specifies a value that may or may not exist.

Mangrove offers support for optional types where `T` is a type discussed on this page. This gives Mangrove support for dynamic, flexible schemas, which will be discussed in a [later section](/2-models/dynamic-schemas).

## ObjectIDs

One of the fundamental types in MongoDB is the {{% a_blank "ObjectID" "https://docs.mongodb.com/manual/reference/bson-types/#objectid" %}}. ObjectIDs are the default type used for the `_id` field in documents, so you may want to use them in your project.

The MongoDB C++ Driver offers the {{% a_blank "`bsoncxx::oid`" "http://mongodb.github.io/mongo-cxx-driver/classbsoncxx_1_1oid.html" %}}, which is a simple class that can be used to create and compare ObjectIDs. Mangrove offers serialization support for `bsoncxx::oid` so you can include them in your models as well. 

## BSON Types

The MongoDB C++ Driver offers a library called `libbsoncxx` that contains a set of utilities for using and manipulating BSON in C++. One of these utilities is a set of C++ types that cover every supported type in the {{% a_blank "BSON Specification" "http://bsonspec.org/spec.html" %}}.

{{% notice warning %}}
While the following types are all technically supported, we do not recommend using them if you can accomplish the same result with one of types discussed in the other sections. Many of the following types are dangerous and only used internally by MongoDB.
{{% /notice %}}

These types, which are all defined in {{% a_blank "`bsoncxx/types.hpp`" "http://mongodb.github.io/mongo-cxx-driver/types_8hpp_source.html" %}}, are all supported as serializable by Mangrove.

The following types, which are in the `bsoncxx::types::` namespace are supported without any further configuration:

* `b_double`
* `b_bool`
* `b_date`
* `b_int32`
* `b_int64`
* `b_undefined`
* `b_null`
* `b_timestamp`
* `b_minkey`
* `b_maxkey`
* `b_oid`
    - Not to be confused with the `bsoncxx::oid` discussed in the previous section.

### BSON "View" Types

Some of the types defined in the `bsoncxx::types::` namespace are non-owning "views" to their underlying data, meaning that they can be dangerous if the address they pointing to is freed from memory. The following types are BSON "views":

* `b_utf8`
* `b_document`
* `b_array`
* `b_binary`
* `b_regex`
* `b_dbpointer`
* `b_code`
* `b_codewscope`
* `b_symbol`

Mangrove only supports these types if, in the class containing them, you inherit from `bson_mapper::UnderlyingBSONDataBase`. When data is loaded from the database and deserialized into this class, {{% a_blank "`UnderlyingBSONDataBase`" "/api/html/classbson__mapper_1_1UnderlyingBSONDataBase.html" %}} holds a `std::shared_ptr` to the original BSON data from which it was deserialized. This ensures that any BSON view types in the class will never point to deallocated memory after being read from the database.

