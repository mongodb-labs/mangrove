+++
next = "/1-basics/installation"
prev = "/1-basics"
title = "What is Mangrove?"
toc = true
weight = 1

+++

Mangrove is a C++ **object document mapper (ODM)** for MongoDB. An **ODM** lets you map classes in an object-oriented programming language to collections in a document-oriented database like MongoDB. 

If you've ever used an **object relational mapper (ORM)**, an **ODM** is quite similar, except that the underlying structure of the data in the database is actually very similar to the structure of your objects.

If you know how to create and manipulate classes in C++, you can easily get started with Mangrove!

```cpp

class BlogEntry : public mangrove::model<BlogEntry> {
   public:
    std::string title;
    std::string contents;
    std::string author;
    std::chrono::system_clock::time_point time_posted;

    // Register fields with Mangrove.
    MANGROVE_MAKE_KEYS_MODEL(BlogEntry, 
                             MANGROVE_NVP(title),
                             MANGROVE_NVP(contents),
                             MANGROVE_NVP(author),
                             MANGROVE_NVP(time_posted))
};

int main() {
    mongocxx::instance{};
    mongocxx::client conn{mongocxx::uri{}};

    // Map the BlogEntry class to the 'entries' collection
    // in the 'my_blog' database.
    auto db = conn["my_blog"];
    BlogEntry::setCollection(db["entries"]);

    // Create a new entry just like any other C++ object.
    BlogEntry new_entry{"Check out Mangrove",
                        "It's really cool!",
                        "Ben Franklin",
                        std::chrono::system_clock::now()}


    // Save the entry in the collection.
    new_entry.save();

    // There is now a document in the 'entries' collection
    // and it looks like this:
    // {
    //    "_id" : ObjectId("577d5b36e24939f3b82e2331"),
    //    "title" : "Check out Mangrove!",
    //    "contents" : "It's really cool",
    //    "author" : "Ben Franklin",
    //    "time_posted" : ISODate("2016-07-15T19:25:42.073Z")
    // }
}


```
