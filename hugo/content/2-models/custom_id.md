+++
next = "/2-models/dynamic-schemas"
prev = "/2-models/allowed-types"
title = "Custom _id Type"
toc = true
weight = 3

+++

As we mentioned in the chapter's [Introduction](/2-models/introduction), Mangrove **models** automatically include an `_id` field that is automatically an ObjectID of type `bsoncxx::oid`.

There are various reasons that you might not want to use an ObjectId for the `_id` field. Therefore, Mangrove supports the customization of the `_id` type via a second template parameter to `mangrove::model`.

Below is an exampe of a model that uses std::string as an `_id` instead of an ObjectID:

```cpp
class User : public mangrove::model<User, std::string> {
	std::string username;
	std::string password_hash;
}
```

The resulting BSON document may look something like this:
```json
{
	"_id" : "alan.turing@mongodb.com",
	"username" : "aturing",
	"password_hash" :
		 "e9f5bd2bae1c70770ff8c6e6cf2d7b76"
}
```

## Supported Types

Mangrove supports any type discussed in [Allowed Types](/2-models/allowed-types) as `_id`'s type, except for container types, `b_regex`, and `b_array`. This means that you can even use an embedded document as your `_id` type!

You can read {{% a_blank "this article" "https://docs.mongodb.com/manual/core/document/#document-id-field" %}} in the MongoDB Manual to learn more about the exact limitations of the `_id` field.

