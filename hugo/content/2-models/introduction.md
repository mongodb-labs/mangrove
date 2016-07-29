+++
next = "/2-models/allowed-types"
prev = "/2-models"
title = "Introduction"
toc = true
weight = 1

+++

A **model** is the fundamental way you access data in MongoDB with Mangrove. Models lets you treat C++ classes as objects you can store in the database. When you make a class a model, it is given the semantics of a MongoDB collection, and has access to all of {{% a_blank "these static methods" "/api/html/classmangrove_1_1model.html" %}}.

## Creating a Model

You can turn any *default-constructible* C++ class into a model by having it inherit publicly from the `mangrove::model<T>` base class, where `T` is type of the class you are making a model.

```cpp
// Example of inheritance required for model
class Message : mangrove::model<Message> {
    bsoncxx::oid author_id;
    std::string content;
    std::chrono::system_clock::time_point time_sent;
}
```

This inheritance gives the inheriting class access to the interface specified {{% a_blank "here" "/api/html/classmangrove_1_1model.html" %}}. The inheritance also gives you access to the `_id` variable. Since every document stored in a MongoDB database contains an `_id`, the model stores one automatically for you. By default this of type {{% a_blank "`bsoncxx::oid`" "http://mongodb.github.io/mongo-cxx-driver/classbsoncxx_1_1oid.html" %}}, but you can customize it. We'll discuss how to do this in a [later section](/2-models/custom_id).

## Specifying Fields to Serialize

Before you can use a model, you must specify which of its fields you want to be included when an instance of a model is serialized and saved to the database. You can do this with the `MANGROVE_MAKE_KEYS_MODEL` macro.

This macro accepts the class you are registering as a model, as well as a variadic list of the fields you want to include, wrapped as name-value pairs (**NVPs**).

```cpp
// Example of registering fields in model
class Message : mangrove::model<Message> {
    bsoncxx::oid author_id;
    std::string content;
    std::chrono::system_clock::time_point time_sent;
    int dont_include_me;

    MANGROVE_MAKE_KEYS_MODEL(Message,
                             MANGROVE_NVP(author_id),
                             MANGROVE_NVP(content),
                             MANGROVE_NVP(time_sent))
}
```

The `MANGROVE_NVP` macro specifies that you want its argument serialized in such a way that its **name** or **key** is the name of the variable. For example, in the above code sample, the resulting saved BSON document would look something like the following:

```json
{
    "_id" : ObjectId("576d7238e249391db3271e31"),
    "author_id" : ObjectId("576d5145e2493917691144f1"),
    "content" : "Hey, did you see how cool Mangrove is?!",
    "time_sent" : ISODate("2016-07-01T14:42:06.018Z")
}
```

{{% notice info %}}
While there is no limitation to the complexity of the classes you can turn into a model, there are limitations regarding which types are serializable and can be included in the `MANGROVE_MAKE_KEYS_MODEL` macro. These limitations are discussed in the [next section](/2-models/allowed-types). 
{{% /notice %}}

If you don't want the name of the field to be exactly the same as the name of your variable, you can use the macro `MANGROVE_CUSTOM_NVP` to specify a custom name for the resulting BSON element of a particular field.

```cpp
// Example of registering fields in model with custom names
class Message : mangrove::model<Message> {
    bsoncxx::oid author_id;
    std::string content;
    std::chrono::system_clock::time_point time_sent;
    int dont_include_me;

    MANGROVE_MAKE_KEYS_MODEL(Message,
                             MANGROVE_CUSTOM_NVP("a_id", author_id),
                             MANGROVE_CUSTOM_NVP("c", content),
                             MANGROVE_NVP(time_sent))
}
```

In the above example, the resulting BSON documents would now look like the following:

```json
{
    "_id" : ObjectId("576d7238e249391db3271e31"),
    "a_id" : ObjectId("576d5145e2493917691144f1"),
    "c" : "Hey, did you see how cool Mangrove is?!",
    "time_sent" : ISODate("2016-07-01T14:42:06.018Z")
}
```

{{% notice tip %}}

If you have extremely large collections and want to save some disk space and bandwidth, you can use custom NVPs to shorten the names of fields associated with large variable names to a single-letter.

{{% /notice %}}

### Public vs. Private Members

In Mangrove, you can serialize both public and private class members. The only caveat with this is that you won't be able to access private members in [queries](/3-queries) or [updates](/4-updates) that you build outside of the class. If you want to keep your models encapsulated, build all of the queries and updates you'll be using inside your model's member functions.

## Linking With the Database

The last step you must take before you can use a model is to register it with a MongoDB database. Mangrove itself doesn't handle the connection to the database, that responsibility is held by the C++ Driver, which provides the `mongocxx::client` class. You can learn more about that class in the C++ Driver's {{% a_blank "API Documentation""http://mongodb.github.io/mongo-cxx-driver/classmongocxx_1_1client.html"%}}. 

In the following example, we'll use a connection to the database hosted at `localhost` on port 27017.

```cpp
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

int main() {
    mongocxx::instance{};
    mongocxx::client 
        conn{mongocxx::uri{"mongodb://localhost:27017"};

    Message::setCollection(conn["testdb"]["messages"]);
}
```

All models have a static function `setCollection()` that lets you specify in which collection to save and retrieve instances of that particular model. It accepts a `mongocxx::collection`, which is accessible via the `mongocxx::client`. The code above sets `Message`'s collection to the `"entries"` collection in the `"my_blog"` database hosted at `"mongodb://localhost:27017"`.

{{% notice warning %}}
The `mongocxx::client` class is *not thread-safe*. To get around this, the `mangrove::model` class provides thread-local storage for the collection associated with a particular model. You can make your applications thread-safe by calling the model's `setCollection()` function on each thread with a collection from a new `mongocxx::client`. You can read more about the C++ Driver's thread safety {{% a_blank here "https://github.com/mongodb/mongo-cxx-driver/wiki/Library-Thread-Safety" %}}.
{{% /notice %}}

## Active Record Manipulation

The simplest way to add, modify, and remove objects of a particular model is through the {{% a_blank "active record pattern" "https://en.wikipedia.org/wiki/Active_record_pattern" %}}. When you create an instance of a model class, that instance can be saved to the database with its `save()` method, and it can also be removed from the database with its `remove()` method.

```cpp
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

int main() {
    mongocxx::instance{};
    mongocxx::client 
        conn{mongocxx::uri{"mongodb://localhost:27017"};

    Message::setCollection(conn["testdb"]["messages"]);

    Message msg{bsoncxx::oid("576d5145e2493917691144f1"),
                "Hey, did you see how cool Mangrove is?!".
                std::chrono::system_clock::now(), 42};

    // Inserts the message to the database.
    msg.save();

    // Updates the message in the database.
    msg.content = "I edited my message!";
    msg.save();

    // Removes the message from the database.
    msg.remove();
}
```

This might not seem very interesting since we've only used the active record model on objects have been created within the program. However, the `save()` and `remove()` methods are also available with model objects that get returned from queries. We'll go more in depth about queries in Mangrove in the [next chapter](/3-queries), but the following example demonstrates how we can use the active record model to delete every message from a particular user:

```cpp
for(auto msg : Message::find(MANGROVE_KEY(Message::author_id) == bsoncxx::oid("576d5145e2493917691144f1"))) {
    msg.remove();
}
```
{{% notice note %}}
While the above code works, a more efficient way of achieving the same result is through a bulk delete with the model's `deleteMany()` method.
{{% /notice %}}
