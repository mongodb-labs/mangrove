+++
next = "/4-updates/"
prev = "/3-queries/introduction"
title = "Operators"
toc = true
weight = 1

+++

The following is a list of the available **query operators** for the Mangrove query builder.
This page follows the structure of the
{{% a_blank "MongoDB query operator documentation" "https://docs.mongodb.com/manual/reference/operator/query/" %}}.

### Comparison Operators
The following operators can be expressed using built-in C++ operators such as `<` and `!=`.
They work on any field type, and the right-hand value must be the same type as the field.
Free functions are also provided in the `mangrove` namespace that can be used in lieu of the overloaded
operators.

{{% notice tip %}}
When working with array fields, one can also compare them to values that are the type of the array's elements.
This provides similar functionality to using the `$elemMatch` operator, with a few caveats.
For more information on this, and an example, see the Mangrove documentation for `$elemMatch`, under the [Array Operators]({{< relref "#array-operators" >}}).
{{% /notice %}}

- `{{% a_blank "$eq" "https://docs.mongodb.com/manual/reference/operator/query/eq/" %}}`
 ```cpp
 // { age: {$eq: 21} }
 auto results = User::find(MANGROVE_KEY(User::age) == 21);
 auto results = User::find(mangrove::eq(MANGROVE_KEY(User::age), 21));
 ```

- `{{% a_blank "$gt" "https://docs.mongodb.com/manual/reference/operator/query/gt/" %}}`
 ```cpp
 // { age: {$gt: 21} }
 auto results = User::find(MANGROVE_KEY(User::age) > 21);
 auto results = User::find(mangrove::gt(MANGROVE_KEY(User::age), 21));
 ```

- `{{% a_blank "$gte" "https://docs.mongodb.com/manual/reference/operator/query/gte/" %}}`
 ```cpp
 // { age: {$gte: 21} }
 auto results = User::find(MANGROVE_KEY(User::age) >= 21);
 auto results = User::find(mangrove::gte(MANGROVE_KEY(User::age), 21));
 ```

- `{{% a_blank "$lt" "https://docs.mongodb.com/manual/reference/operator/query/lt/" %}}`
 ```cpp
 // { age: {$lt: 21} }
 auto results = User::find(MANGROVE_KEY(User::age) < 21);
 auto results = User::find(mangrove::lt(MANGROVE_KEY(User::age), 21));
 ```

- `{{% a_blank "$lte" "https://docs.mongodb.com/manual/reference/operator/query/lte/" %}}`
 ```cpp
 // { age: {$lte: 21} }
 auto results = User::find(MANGROVE_KEY(User::age) <= 21);
 auto results = User::find(mangrove::lte(MANGROVE_KEY(User::age), 21));
 ```

- `{{% a_blank "$ne" "https://docs.mongodb.com/manual/reference/operator/query/ne/" %}}`
 ```cpp
 // { age: {$ne: 21} }
 auto results = User::find(MANGROVE_KEY(User::age) != 21);
 auto results = User::find(mangrove::ne(MANGROVE_KEY(User::age), 21));
 ```

The following operators are provided as member functions of the fields which are returned from the
`MANGROVE_KEY` and other macros.
If the field is a scalar, then the given right hand side must be an array containing the same type.
If the field is an array, then the given right hand side must be a similar array, containing the same type.

- `{{% a_blank "$in" "https://docs.mongodb.com/manual/reference/operator/query/in/" %}}`
 ```cpp
 // { age: {$in: [16, 18, 21]} }
 int ages = std::vector<int>{16, 18, 21}
 auto results = User::find(MANGROVE_KEY(User::age).in(ages));
 ```

- `{{% a_blank "$nin" "https://docs.mongodb.com/manual/reference/operator/query/nin/" %}}`
 ```cpp
 // { age: {$nin: [16, 18, 21]} }
 int ages = std::vector<int>{16, 18, 21}
 auto results = User::find(MANGROVE_KEY(User::age).nin(ages));
 ```

### Logical Operators
The `$or` and `$and` operators can be used to chain different sub-queries together.
When using both at once, the `&&` and `||` follow their usual C++ precedence rules.

- `{{% a_blank "$or" "https://docs.mongodb.com/manual/reference/operator/query/or/" %}}`
 ```cpp
 // { $or: [ {age: {$gt: 65}}, {age: {$lt: 5}} ] }
 auto results = User::find((MANGROVE_KEY(User::age) > 65 || MANGROVE_KEY(User::age) < 5));
 ```

- `{{% a_blank "$and" "https://docs.mongodb.com/manual/reference/operator/query/and/" %}}`
 ```cpp
 // { $and: [ {age: {$gt: 65}}, {age: {$lt: 5}} ] }
 auto results = User::find((MANGROVE_KEY(User::age) > 65 && MANGROVE_KEY(User::age) < 5));
 ```

- `{{% a_blank "$not" "https://docs.mongodb.com/manual/reference/operator/query/not/" %}}`

    The `$not` operator is applied to individual comparisons
    (as opposed to boolean expressions using the above operators).
    The C++ `!` operator is used for this.

     ```cpp
     // { age: {$not: {$lt: 21}} }
     auto results = User::find(!(MANGROVE_KEY(User::age) < 21));
     ```

- `{{% a_blank "$nor" "https://docs.mongodb.com/manual/reference/operator/query/nor/" %}}`

    The `$nor` operator behaves similarly to the `$and` and `$or` operators,
    except that there is no built-in C++ `NOR` operator. As such, it is provided as a free function that takes
    a list of query expressions.

     ```cpp
     // { $nor: [ {age: {$gt: 65}}, {age: {$lt: 5}} ] }
     auto results = User::find(mangrove::nor(MANGROVE_KEY(User::age) > 65, MANGROVE_KEY(User::age) < 5));
     ```

### Element Operators

- `{{% a_blank "$exists" "https://docs.mongodb.com/manual/reference/operator/query/exists/" %}}`

    The `$exists` operator is provided as a member function on fields that takes a boolean
    argument. It is only enabled for **optional** fields.

    ```cpp
    // { age: {$exists: true} }
    auto results = User::find(MANGROVE_KEY(User::age).exists(true));
    ```

- The `{{% a_blank "$type" "https://docs.mongodb.com/manual/reference/operator/query/type/" %}}`
    operator is not currently available in the Mangrove query builder,
    since to work with serialization,
    the types of fields must correspond to the type defined in the C++ class.

### Evaluation Operators

- `{{% a_blank "$mod" "https://docs.mongodb.com/manual/reference/operator/query/mod/" %}}`

    This operator is provided as a member function on fields,
    and accepts two parameters: a divisor and a remainder.
    It is only enabled for fields with numeric types.

    ```cpp
    // { age: {$mod: {$divisor: 10, $remainder: 3}} }
    auto results = User::find(MANGROVE_KEY(User::age).mod(10, 3));
    ```

- `{{% a_blank "$regex" "https://docs.mongodb.com/manual/reference/operator/query/regex/" %}}`

    This operator is provided as a member function on fields,
    and accepts two parameters: a regex string and an options string.
    It is only enabled for string fields.

    ```cpp
    // Match users who live on a numbered street, like "14th st.":
    // { "addr.street": {$regex: "\d+th st\.?", $options: "i"} }
    auto results = User::find(MANGROVE_CHILD(User, addr, street).regex("\\d+th st\\.?", "i"));
    ```

- `{{% a_blank "$text" "https://docs.mongodb.com/manual/reference/operator/query/text/" %}}`

    This operator performs a search on a text index in the database.
    It is provided as a free function that accepts a text query as a string,
    as well as the optional `language`, `case_sensitive`, and `diacritic_sensitive` parameters
    as per the MongoDB documentation.
    These parameters can be passed in as `optional` values,
    or specified using "fluent" setter functions:

    ```cpp
    // {
    //   $text:
    //     {
    //       $search: "boulevard",
    //       $language: "en",
    //       $caseSensitive: false,
    //       $diacriticSensitive: false
    //     }
    // }
    auto results = User::find(mangrove::text("boulevard").language("en")
                                                         .case_sensitive(false)
                                                         .diacritic_sensitive(false));
    ```

{{% notice warning %}}
There are {{% a_blank "restrictions" "https://docs.mongodb.com/manual/reference/operator/query/text/#restrictions" %}} on the syntax of the `$text` operator that Mangrove does not check.
Refer to the MongoDB documentation to make sure that the `$text` operator is being used correctly.
Also, fields **must** be text-indexed for the `$text` operator to work on them.
{{% /notice %}}

- `{{% a_blank "$where" "https://docs.mongodb.com/manual/reference/operator/query/where/" %}}`

    *Mangrove does not provide a `$where` operator.*


### Geospational Operators
*Mangrove does not currently support geospatial operators, although support is planned in the future, along with a dedicated GeoJSON C++ data type.*

### Array Operators

For the following code samples, assume that a User has a field `scores`, which contains an array of integers.

- `{{% a_blank "$all" "https://docs.mongodb.com/manual/reference/operator/all/" %}}`

    This operator is provided as a member function on fields.
    It accepts an array of values that are the same type as the elements of the field's array.
    This is only enabled for fields which are iterable (i.e. arrays).

    ```cpp
    // { scores: {$all: [100, 500, 1000]} }
    auto results = User::find(MANGROVE_KEY(User::scores).all(std::vector<int>{100, 500, 1000}));
    ```

- `{{% a_blank "$elemMatch" "https://docs.mongodb.com/manual/reference/operator/elemMatch/" %}}`
    
    This operator accepts a query, and returns documents if the elements inside a field's array match
    the given queries. The queries can be constructed in Mangrove using the same syntax as top-level queries.

    For instance, if a User document contains a field `past_homes`, which is an array of addresses,
    one could query it like so:

    ```cpp
    // Find users that have lived, at some point, in Maryland.
    // { past_homes: {$elemMatch: {state: "MD"}} }
    auto results = User::find(MANGROVE_KEY(User::past_homes).elem_match(MANGROVE_KEY(Address::state) == "MD"));
    ```

    Using `$elemMatch` on scalar arrays is a bit tricky, since the scalars don't have field names to refer to.
    Instead, one uses the `MANGROVE_KEY_ELEM(...)` macro to create a field object that refers to
    the *elements* of an array. (The equivalent for `MANGROVE_CHILD` is `MANGROVE_CHILD_ELEM`.)
    One can also use `MANGROVE_KEY(...).element()` and `MANGROVE_CHILD(...).element()`, respectively.

    ```cpp
    // Find users with a score greater than 9000.
    // { scores: {$elemMatch: {$gt: 9000}} }

    // The following are equivalent:
    auto results = User::find(MANGROVE_KEY(User::scores).elem_match(MANGROVE_KEY_ELEM(User::score) > 9000));
    results = User::find(MANGROVE_KEY(User::scores).elem_match(MANGROVE_KEY(User::score).element() > 9000));
    ```

    In the MongoDB query language, if you're only checking one condition in `$elemMatch`,
    you can omit the operator itself and compare the array and value directly:
    `{num_array: {$gt: 5}}` is equivalent to `{num_array: {$elemMatch: {$gt: 5}}}`.
    Mangrove mirrors this syntax by allowing you to directly compare values and array fields:

    ```cpp
    // The following queries are identical. They both return documents in which 'scores' contains at least one
    // element greater than 9000.
    auto results = User::find(MANGROVE_KEY(User::scores).elem_match(MANGROVE_KEY_ELEM(User::score) > 9000));
    auto results = User::find(MANGROVE_KEY(User::scores) > 9000);
    ```

    Note that these two ways of querying arrays are the same **only when specifying one condition**.
    With more conditions, there can be semantic differences.
    When a query has several conditions,
    `$elemMatch` retrieves all documents where at least one array element matches *all* the given condition,
    while specifying several conditions using direct "array-to-scalar" comparison
    returns documents that can contain *several different elements*, that each match an individual condition.
    Further clarification on this is given in the MongoDB documentation for {{% a_blank "querying on arrays"  "https://docs.mongodb.com/manual/tutorial/query-documents/#query-on-arrays" %}}

{{% notice note %}}
When specifying conditions on the fields of documents in an array in the `$elemMatch` operator,
the fields are referred to as top-level fields.
In the example above, the `state` field is used as `MANGROVE_KEY(Adress::state)`,
as opposed to `MANGROVE_CHILD(User, past_homes, state)`.
This mirrors the syntax of MongoDB queries themselves.
The equivalent BSON would refer to the `state` field simply as `state`, not `"past_homes.state"`.
{{% /notice %}}

{{% notice warning %}}
Mangrove does not check if you use `.element()` or `MANGROVE_KEY_ELEM` outside of `elem_match(...)`.
This will cause a runtime error when the MongoDB server receives an invalid query.
{{% /notice %}}

- `{{% a_blank "$size" "https://docs.mongodb.com/manual/reference/operator/size/" %}}`

    This operator is provided as a member function on fields.
    It accepts a single argument, which is the array size to check.
    The operator is only enabled for fields which are iterable (i.e. arrays).

    ```cpp
    // { scores: {$size: 10} }
    auto results = User::find(MANGROVE_KEY(User::scores).size(10));
    ```

### Bitwise Operators

The following operators query for documents using bit operations on fields.
Each operator has two versions: one that accepts a single *mask*,
either as an integer or a BSON `b_binary` type, and another that takes a variadic number of
bit *positions* as unsigned integers.

These operators are only enabled for integral or `b_binary` types.

For the following examples, assume that the User class has an integer field `bitvector` that
represents a set of bits that we want to do bitwise operations on.

- `{{% a_blank "$bitsAllSet" "https://docs.mongodb.com/manual/reference/operator/query/bitsAllSet/" %}}`

    ```cpp
    // We want to compare the `bitvector` field with the value 0b00010101.
    // The following two queries are equivalent:

    // { scores: {$bitsAllSet: 21} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAllSet(21));
    // { scores: {$bitsAllSet: [0, 2, 4]} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAllSet(0, 2, 4));
    ```

- `{{% a_blank "$bitsAnySet" "https://docs.mongodb.com/manual/reference/operator/query/bitsAnySet/" %}}`

    ```cpp
    // We want to compare the `bitvector` field with the value 0b00010101.
    // The following two queries are equivalent:

    // { scores: {$bitsAnySet: 21} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAnySet(21));
    // { scores: {$bitsAnySet: [0, 2, 4]} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAnySet(0, 2, 4));
    ```

- `{{% a_blank "$bitsAllClear" "https://docs.mongodb.com/manual/reference/operator/query/bitsAllClear/" %}}`

    ```cpp
    // We want to compare the `bitvector` field with the value 0b00010101.
    // The following two queries are equivalent:

    // { scores: {$bitsAllClear: 21} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAllClear(21));
    // { scores: {$bitsAllClear: [0, 2, 4]} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAllClear(0, 2, 4));
    ```

- `{{% a_blank "$bitsAnyClear" "https://docs.mongodb.com/manual/reference/operator/query/bitsAnyClear/" %}}`

    ```cpp
    // We want to compare the `bitvector` field with the value 0b00010101.
    // The following two queries are equivalent:

    // { scores: {$bitsAnyClear: 21} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAnyClear(21));
    // { scores: {$bitsAnyClear: [0, 2, 4]} }
    auto results = User::find(MANGROVE_KEY(User::bitvector).bitsAnyClear(0, 2, 4));
    ```
