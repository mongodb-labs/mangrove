+++
next = "/5-the-bson-mapper/"
prev = "/4-updates/introduction"
title = "Operators"
toc = true
weight = 1

+++

The following is a list of the available **update operators** for the Mangrove expression builder.
This page follows the structure of the
{{% a_blank "MongoDB update operator documentation" "https://docs.mongodb.com/manual/reference/operator/update/" %}}.

### Field Update Operators

- `{{% a_blank "$inc" "https://docs.mongodb.com/manual/reference/operator/update/inc/" %}}`

    The `$inc` operator is provided as the C++ `+=` and `-=` operators.
    The unary `++` and `--` can also be used.
    It is only enabled for fields with a numeric type.

    ```cpp
    // Increment the age of users in the database.
    // The following three statements are the same, and produce the BSON:
    // { $inc: {age: 1}}
    auto res = User::update_many({}, MANGROVE_KEY(User::age) += 1);
    res = User::update_many({}, MANGROVE_KEY(User::age)++);
    res = User::update_many({}, ++MANGROVE_KEY(User::age));

    // Decrement the time left in the free trial of some product.
    // The following three statements are the same, and produce the BSON:
    // { $inc: {trial_days_left: -1}}
    auto res = User::update_many({}, MANGROVE_KEY(User::trial_days_left) -= 1);
    res = User::update_many({}, MANGROVE_KEY(User::trial_days_left)--);
    res = User::update_many({}, --MANGROVE_KEY(User::trial_days_left));
    ```

- `{{% a_blank "$mul" "https://docs.mongodb.com/manual/reference/operator/update/mul/" %}}`

    The `$mul` operator is provided as the C++ `*=` operator.
    It is only enabled for fields with a numeric type.

    ```cpp
    // Double the scores of users in a game.
    // { $mul: {score: 2}}
    auto res = User::update_many({}, MANGROVE_KEY(User::score) *= 2);
    ```

- `{{% a_blank "$rename" "https://docs.mongodb.com/manual/reference/operator/update/rename/" %}}`

    *Mangrove does not provide the `$rename` operator, since changing the name of fields created during serialization is not possible.*

- `{{% a_blank "$setOnInsert" "https://docs.mongodb.com/manual/reference/operator/update/setOnInsert/" %}}`

    This operator is provided as a member function on the fields.
    The given value must be the same type as the field (or implicitly cast to it).

    ```cpp
    // Reset the score of a user, or create a new one.
    // Note the use of the "upsert" option, to enable insertion of a new document if necessary.
    // { $set: {score: 0}, $setOnInsert: {hours_played: 0} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     (MANGROVE_KEY(User::score) = 0,
                                      MANGROVE_KEY(User::hours_played).set_on_insert(0)),
                                     mongocxx::options::update{}.upsert(true));
    ```

- `{{% a_blank "$set" "https://docs.mongodb.com/manual/reference/operator/update/set/" %}}`

    The `$set` operator is provided as the C++ assignment operator `=`.
    The given value must be the same type as the field (or implicitly cast to it).

    ```cpp
    // Edit the username of a user
    // { $set: {username: "brohemian_raphsody"}}
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                    MANGROVE_KEY(User::username) = "brohemian_raphsody");
    ```

- `{{% a_blank "$unset" "https://docs.mongodb.com/manual/reference/operator/update/unset/" %}}`

    This operator is provided as a member function on fields that takes no arguments.
    The field it is used on must be an `optional` field.
    Otherwise, an error is raised at compile time.

    ```cpp
    // Remove the "current_school" field from a user who has graduated:
    // {$unset: {current_school: ""}}
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::current_school).unset());
    ```

- `{{% a_blank "$min" "https://docs.mongodb.com/manual/reference/operator/update/min/" %}}`

    This operator is provided as a member function on fields.
    The given value must be the same type as the field (or implicitly cast to it).

    ```cpp
    // Update a user's lowest score in a game.
    // {$min: {lowest_score: 45}}
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::lowest_score).min(45));
    ```

- `{{% a_blank "$max" "https://docs.mongodb.com/manual/reference/operator/update/max/" %}}`

    This operator is provided as a member function on fields.
    The given value must be the same type as the field (or implicitly cast to it).

    ```cpp
    // Update a user's high score in a game.
    // {$max: {lowest_score: 1000}}
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::high_score).max(1000));
    ```

- `{{% a_blank "$currentDate" "https://docs.mongodb.com/manual/reference/operator/update/currentDate/" %}}`

    To set a field to the current date using the `$currentDate` operator, one can use assign the reserved value `mangrove::current_date` to a field using `=`. This is only enabled for date or timestamp fields.
    The `$type` argument to the `$currentDate` operator is determined by the type of the field itself.

    ```cpp
    // Update a user's "last played" date.
    // { $currentDate: {last_played: {$type: "date"}} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::last_played) = mangrove::current_date);
    ```

### Array Update Operators
- `{{% a_blank "$" "https://docs.mongodb.com/manual/reference/operator/update/positional/" %}}`

    The `$` operator is used to update the first element in an array that matches the given query.
    This is done in Mangrove by using the `.first_match()` member function on name-value-pairs.
    The result is then used in an update expression as if it were a regular name-value pair.

    This operator is only enabled for fields that are arrays.

    ```cpp
    // Double a players' first score over 1000.
    // { $mul: {"scores.$": 2}}
    auto res = User::update_many(MANGROVE_KEY(User::scores).elem_match(MANGROVE_ELEM(User::scores) > 1000),
                                      MANGROVE_KEY(User::scores).first_match() *= 2);
    ```

- `{{% a_blank "$addToSet" "https://docs.mongodb.com/manual/reference/operator/update/addToSet/" %}}`

    This operator is available as a member function on the fields objects.
    It is only enabled for fields that are arrays.
    There are two versions: one that takes a single value to be added to the array,
    and another that takes an iterable containing multiple values to be added.
    The second one is equivalent to using the `$each` modifier with `$addToSet`.

    ```cpp
    // Add a movie to a user's set of purchased movies.
    // { $addToSet: {purchased_movies: "The Matrix"} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                      MANGROVE_KEY(User::purchased_movies).add_to_set("The Matrix"));
    // Add several movies at once:
    // { $addToSet: {purchased_movies:
    //                  {$each: ["The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"]}
    //              } }
    auto movies = std::vector<std::string>{"The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"};
    res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                 MANGROVE_KEY(User::purchased_movies).add_to_set(movies));
    ```

- `{{% a_blank "$pop" "https://docs.mongodb.com/manual/reference/operator/update/pop/" %}}`

    This operator is provided as a member function on the name-value-pairs.
    It accepts a single parameter, a boolean `last` that determines whether an element should be removed
    from the end (if `last == true`) of the list, or the start (if `last == false`).

    ```cpp
    // Remove the earliest score of some user.
    // { $pop: {scores: -1} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                MANGROVE_KEY(User::scores).pop(false));
    ```

- `{{% a_blank "$pull" "https://docs.mongodb.com/manual/reference/operator/update/pull/" %}}`

    This operator is provided as a member function on the name-value-pairs.
    There are two options: one which takes a value to be removed from the list,
    and another which takes a query and removes values that match the given query.

    In the second one, the syntax for queries is similar to that of the `$elemMatch` operators.
    One can use find queries as they would at the top level (i.e. when passing to `find(...)`),
    except that scalar arrays need to use the `MANGROVE_ELEM` macro to refer to elements.

    `$pull` is only enabled for fields that are arrays.

    ```cpp
    // Remove the first matrix movie from a user's purchased movies.
    // { $pull: {purchased_movies: "The Matrix"} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                      MANGROVE_KEY(User::purchased_movies).pull("The Matrix"));

    // Remove any and all matrix movies from a user's purchased movies.
    // { $pull: {purchased_movies: {$regex: "matrix", $options: "i"}} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::purchased_movies).pull(MANGROVE_ELEM(User::purchased_movies)).regex("matrix", "i"));
    ```

- `{{% a_blank "$pullAll" "https://docs.mongodb.com/manual/reference/operator/update/pullAll/" %}}`

    This operator is provided as a member function on the fields.
    It accepts an iterable argument, that contains a list of values to remove from the field's array.
    This operator is only enabled for fields that are arrays.

    ```cpp
    // Remove the three matrix movies from a user's purchased movies.
    // { $pullAll: {purchased_movies: ["The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"]} }
    auto movies = std::vector<std::string>{"The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"};
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::purchased_movies).pullAll(movies));
    ```

- `{{% a_blank "$pushAll" "https://docs.mongodb.com/manual/reference/operator/update/pushAll/" %}}`

    *`$pushAll` is deprecated, one should use `$push` instead.*

- `{{% a_blank "$push" "https://docs.mongodb.com/manual/reference/operator/update/push/" %}}`

    The `$push` operator is provided as a member function on the fields.
    There are two versions: one which takes a single value to push onto the array,
    and another which takes an iterable, and pushes all the values onto the array
    using the `$each` modifier.

    This operator is only enabled for fields that are arrays.

    ```cpp
    // Push a single movie to a user's watched_movies field:
    // {$push: {watched_movies: "The Matrix"}}
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::watched_movies).push("The Matrix"));

    // This user watched every Matrix movie in one sitting:
    // {$push: {watched_movies: {$each: ["The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"]}}}
    auto movies = std::vector<std::string>{"The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"};
    res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                MANGROVE_KEY(User::watched_movies).push(movies));
    ```

    As per the MongoDB documentation, `$push` can accept several optional modifiers.
    Modifiers to this operator can either be specified
    as `optional` arguments to the function, or using a "fluent" API.
    This means that parameters are given as `.push(values).slice(...).sort(...).position(...)`.

    - `{{% a_blank "$slice" "https://docs.mongodb.com/manual/reference/operator/update/slice/" %}}`
        --- this modifier takes a 32-bit integer that limits the number of array elements.

        ```cpp
        // Add movies to this user's history, but limit its length to 10.
        // {
        //  $push: {
        //      watched_movies: {$each: ["The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"],
        //                       $slice: 10}
        //      }
        // }
        auto movies = std::vector<std::string>{"The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"};
        res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                    MANGROVE_KEY(User::watched_movies).push(movies).slice(10));
        ```

    - `{{% a_blank "$sort" "https://docs.mongodb.com/manual/reference/operator/update/sort/" %}}`
        --- this modifier takes either an integer that is &plusmn;1, or a *sort expression*.
        An integer argument will sort values by their natural ordering, either in ascending (`+1`)
        or descending (`-1`) order.
        A *sort expression* represents an ordering based on a specific field.
        It can be specified by calling the `.sort(bool ascending)` function on a name-value pair.
        The boolean argument `ascending` orders elements in ascending order if `true`,
        or descending if `false`.

        ```cpp
        // Add movies to this user's history, then sort movies in ascending order by their title.
        // {
        //  $push: {
        //      watched_movies: {$each: ["The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"],
        //                       $sort: 1}
        //      }
        // }
        auto movies = std::vector<std::string>{"The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"};
        res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                    MANGROVE_KEY(User::watched_movies).push(movies).sort(1));
        ```

        If, in the above example, movies were not strings but documents, with a `title` and `rating`
        field, one would be able to choose how to sort them as follows:

        ```cpp
        // Add movies to this user's history, then sort movies by rating in descending order.
        // {
        //  $push: {
        //      watched_movies: {$each: [ {title: "The Matrix", rating: 9},
        //                                {title: "The Matrix: Reloaded", rating: 7},
        //                                {title: "The Matrix: Revolutions", rating: 3} ],
        //                       $sort: {rating: -1}}
        //      }
        // }

        // brace-initialize "Movie" objects.
        auto movies = std::vector<Movie>{{"The Matrix", 9}, {"The Matrix: Reloaded", 7},
                                               {"The Matrix: Revolutions", 3}};
        res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                    MANGROVE_KEY(User::watched_movies).push(movies).sort(MANGROVE_KEY(Movie::rating).sort(false)));
        ```

    - `{{% a_blank "$position" "https://docs.mongodb.com/manual/reference/operator/update/position/" %}}`
        --- this modifier takes an unsigned integer that represents a position in the array.

        ```cpp
        // Add movies to this user's history, but add them at the start (i.e. 0th position)
        // of the array.
        // {
        //  $push: {
        //      watched_movies: {$each: ["The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"],
        //                       $position: 0}
        //      }
        // }
        auto movies = std::vector<std::string>{"The Matrix", "The Matrix: Reloaded", "The Matrix: Revolutions"};
        res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                    MANGROVE_KEY(User::watched_movies).push(movies).position(0));
        ```


### Bitwise Operators

- `{{% a_blank "$bit" "https://docs.mongodb.com/manual/reference/operator/update/bit/" %}}`

    Bitwise updates on fields can be performed using C++'s built-in `&=`, `|=`, and `^=` operators.
    These are only enabled for integer fields.

    ```cpp
    // Perform bitwise operations on a user's age, because, hey, why not?!
    // { $bit: {age: {and: 7}} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                     MANGROVE_KEY(User::age) &= 7);

    // { $bit: {age: {or: 7}} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                      MANGROVE_KEY(User::age) |= 7);

    // { $bit: {age: {xor: 7}} }
    auto res = User::update_one(MANGROVE_KEY(User::username) == "raphofkhan",
                                       MANGROVE_KEY(User::age) ^= 7);
    ```

### Isolation Operators

- `{{% a_blank "$isolated" "https://docs.mongodb.com/manual/reference/operator/update/isolated/" %}}`

    An update expression can be marked as isolated using the `$isolated` operator.
    In Mangrove, is this done by passing the query expression used to match documents to the `mangrove::isolated` function.

    ```cpp
    // Set the 'can_buy_alcohol' flag for all users over the age of 21, but isolate the updates
    // to avoid concurrency issues. This is equivalent to the mongo shell command:
    // db.testcollection.updateMany({age: {$gt: 21}, $isolated: 1},
    //                          {$set: {can_buy_alcohol: true}})
    auto res = User::update_many(mangrove::isolated(MANGROVE_KEY(User::age) >= 21),
                                     MANGROVE_KEY(User::can_buy_alcohol) = true);
    ```
