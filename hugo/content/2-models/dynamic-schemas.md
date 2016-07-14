+++
next = "/3-queries/"
prev = "/2-models/custom_id"
title = "Dynamic Schemas"
toc = true
weight = 4

+++

One of MongoDB's most attractive features is its ability to store "unstructured" data. While this seems irrelevant in the context of an ODM like Mangrove, we can actually take advantage of this.

Let's say that you are storing a collection of web video metadata. You might want to store the number of views, the length of the video in seconds, and the web URL to access the video. If the video is hosted on YouTube, you may also want to store the name of the channel it's hosted on. MongoDB lets you do this by having a "youtube_channel" field that only exists in some documents.

You can accomplish this in Mangrove with `optional` fields.

```cpp
using bson_mapper::stdx::optional;

class VideoMetadata : public mangrove::model<VideoMetadata> {
	int64_t view_count;
	int32_t video_length;

	std::string url;
	optional<std::string> youtube_channel;

	MANGROVE_MAKE_KEYS_MODEL(VideoMetadata,
                             MANGROVE_NVP(view_count),
                             MANGROVE_NVP(video_length),
                             MANGROVE_NVP(url),
                             MANGROVE_NVP(youtube_channel))
}
```

An `optional<T>` in C++ either contains a value of type `T`, or contains nothing at all. When the optional is holding a type, Mangrove will serialize it normally. When it isn't, Mangrove will exclude it entirely.

Here are two sample documents that could be produced by the above model:

```json
{
	"view_count" : 224651625,
	"video_length" : 212,
	"url" : "https://www.youtube.com/watch?v=dQw4w9WgXcQ",
	"youtube_channel" : "RickAstleyVEVO"
}

{
	"view_count" : 305420,
	"video_length" : 193,
	"url" : "https://vimeo.com/53520224"
}
```

You can have as many optional types as you want in a model (and you can even have only optionals) so that your model can be as flexible as your data.