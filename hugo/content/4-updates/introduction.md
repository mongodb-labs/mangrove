+++
next = "/4-updates/operators"
prev = "/4-updates/"
title = "Introduction"
toc = true
weight = 1

+++

Mangrove's expression builder also provides the ability to construct **update** expressions.
The syntax is pretty much the same as for query expressions --- you refer to fields
using `MANGROVE_KEY` and the other macros, and update them using
C++ operators such as `=` and `+=`. Member functions are provided for operators which don't have
a built-in C++ operator analog, such as the `$addToSet` operator.

The following is an example of a bulk update which would edit sales tax info for users who live in
New York:

```cpp
// In the Mongo shell, this would be:
// db.collection.updateMany({"addr.state": "NY"}, {$set: {sales_tax: 0.10}});
auto res = User::update_many(MANGROVE_CHILD(User, addr, state) == "NY", MANGROVE_KEY(User::sales_tax) = 0.10);
```

Note that a query, as seen in {{% a_blank "chapter 3" "/3-queries/" %}},
is given as the first argument, and an update is given as the second.

The next section contains a reference of the available update operators.
