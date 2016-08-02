+++
next = "/2-models"
prev = "/1-basics/installation"
title = "Quick Tour"
toc = true
weight = 3

+++

Now that you have Mangrove installed on your system, we can get started with a quick tour of Mangrove's features. To do this, we'll develop the data model for a very simple persistent blog application. 

{{% notice note %}}
This guide assumes a basic working knowledge of both C++ and MongoDB. You can find a basic introduction to the concepts of MongoDB in {{% a_blank "MongoDB's manual" "https://docs.mongodb.com/manual/introduction/"%}}.
{{% /notice %}}

## Creating a Blog Entry Model

First, we'll want to set up a class to represent a blog entry. A blog entry will have a title, an author, some content, and a posting time. We can represent this in C++ with something like the following:

```cpp
#include <chrono>

class BlogEntry {
public:
    std::string title;
    std::string author;
    std::string content;
    std::chrono::system_clock::time_point time_posted;
}
```

If we want to store blog entries in the database, we'll have to make the `BlogEntry` class a Mangrove **model**. Making a class a **model** gives the class the semantics of a MongoDB **collection**, as well as instance `save()` and `remove()` functions for {{% a_blank "active record" "https://en.wikipedia.org/wiki/Active_record_pattern" %}} style access to the database.

To make `BlogEntry` a **model**, simply have it inherit from `mangrove::model<BlogEntry>`:

```cpp
#include <chrono>

#include <mangrove/model.hpp>

class BlogEntry : public mangrove::model<BlogEntry> {
public:
    std::string title;
    std::string author;
    std::string content;
    std::chrono::system_clock::time_point time_posted;
}
```

Notice that the model class is templated on the class that you want to make a model. This is an example of a {{% a_blank "curiously recurring template parameter" "https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern" %}}, a C++ pattern that lets the parent class have access to the type inheriting from it.

Before we can use the model, we must also specify which fields we want to be included when a `BlogEntry` is saved in the database. We can do this with the `MANGROVE_MAKE_KEYS_MODEL` macro:

```cpp
#include <chrono>

#include <mangrove/nvp.hpp>

class BlogEntry : public mangrove::model<BlogEntry> {
public:
    std::string title;
    std::string contents;
    std::string author;
    std::chrono::system_clock::time_point time_posted;

    MANGROVE_MAKE_KEYS_MODEL(BlogEntry, 
                             MANGROVE_NVP(title),
                             MANGROVE_NVP(contents),
                             MANGROVE_NVP(author),
                             MANGROVE_NVP(time_posted))
}
```

This macro specifies that blog entries should be stored in the database with the title, author, content, and time posted, and where the name of those fields should be the name of the variables. Manually specifying which class members are included when saving to the database lets you have class members that you might not necessarily want to store persistently in the database.

## Registering the Model with a MongoDB Collection

The last step you must take before you can use the `BlogEntry` model is to register the model with a MongoDB database using the MongoDB C++ Driver. In the following example, we'll use a connection to the database hosted at `localhost` on port 27017.

```cpp
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>

int main() {
    mongocxx::instance{};
    mongocxx::client 
        conn{mongocxx::uri{"mongodb://localhost:27017"};

    BlogEntry::setCollection(conn["my_blog"]["entries"]);
}
```

The `BlogEntry` model has a static function `setCollection()` that lets you specify in which collection to save and retrieve instances of `BlogEntry`. It accepts a `mongocxx::collection`, which is accessible via the `mongocxx::client`. The code above sets `BlogEntry`'s collection to the `"entries"` collection in the `"my_blog"` database hosted at `"mongodb://localhost:27017"`.

If you're planning on writing multi-threaded applications with Mangrove, be sure to carefully read the warning [here](/2-models/introduction/#linking-with-the-database).

## Saving Objects

Now that you've set up the model and registered it with a collection, you can start interacting with the database. The simplest way to do this is to create instances of the object and call the save() method. The following very simple function will save a blog entry to the database.

```cpp
void newEntry(std::string title, std::string author, std::string content) { 
    BlogEntry entry{title, author, content,
                    std::chrono::system_clock::now()};
    entry.save();
}
```

This particular function doesn't do anything with the object after saving it to the database, but you are able to modify the `entry` object and call save() again to modify the object and save the changes in the database.

## Querying Objects

Querying objects in Mangrove is just as easy as saving them. When you make `BlogEntry` a **model**, you gave it the static methods `find()` and `findOne()`, which have very similar semantics to {{% a_blank "their" "https://docs.mongodb.com/manual/reference/method/db.collection.find/"%}} {{% a_blank "equivalents" "https://docs.mongodb.com/manual/reference/method/db.collection.findOne/" %}} in the mongo shell. 

The following function prints out the titles of every blog entry by a particular author using a `find()` query:

```cpp
#include <iostream>

#include <mangrove/query_builder.hpp>

void printTitlesBy(std::string author) {
    std::cout << "All blog entries by " << author << ":\n";
    for (auto entry : BlogEntry::find(MANGROVE_KEY(BlogEntry::author) == author)) {
        std::cout << entry.title << "\n";
    }
}
```

There's a lot happening here, but it's very simple once you understand what's going on.

Inside the parameter list for `BlogEntry::find()`, you'll see `MANGROVE_KEY(BlogEntry::author) == author`. In Mangrove, you can build MongoDB queries using the standard C++ comparison and logical operators. This is discussed more in [Chapter 3](/3-queries), but the main thing you have to remember is to wrap the value you are comparing in a `MANGROVE_KEY` macro.

The return type of `BlogEntry::find()` is an iterator of `BlogEntry`s. This makes it very easy to work with the often large cursors of objects that are returned from MongoDB queries. In this case, we are interacting with the cursor via the C++11 {{% a_blank "range-based for loop" "http://en.cppreference.com/w/cpp/language/range-for" %}}.

{{% notice info %}}
The iterator returned by the model's `find()` method has the semantics of a MongoDB **cursor**. This is useful for large queries where you want to periodically read from the database as you're reading from the query results, but this also gives the cursors some interesting behavior such as inactive cursor closure and the lack of cursor isolation. If you want to learn more about these behaviors, check out {{% a_blank "this page" "https://docs.mongodb.com/v3.2/tutorial/iterate-a-cursor/#cursor-behaviors" %}} in the MongoDB manual.  
{{% /notice %}}
