# Mangrove [![Build Status](https://travis-ci.org/mongodb/mongo-cxx-odm.svg?branch=master)](https://travis-ci.org/mongodb/mongo-cxx-odm)

Welcome to Mangrove, the official MongoDB C++ ODM!

Mangrove lets you map your C++ classes to collections in MongoDB for seamless data retrieval and manipulation.

#### Documentation

The documentation for Mangrove is currently hosted at <http://mongodb.github.io/mongo-cxx-odm>.

We generate the documentation with the following commands:

```
hugo -b http://mongodb.github.io/mongo-cxx-odm/ --canonifyURLs=true -s ./hugo -d ../docs
doxygen
```

We then host `docs` by copying its contents to the `gh-pages` branch.

You can generate the documentation on your own by running the `generate_docs.sh` script in this root directory. It requires that you have [Hugo](https://gohugo.io/) and [Doxygen](http://www.stack.nl/~dimitri/doxygen/) installed on your system. If you're on OS X, both can be installed with [Homebrew](http://brew.sh/).

The documentation will be installed in `./docs` with a base URL of `http://localhost:8080/` and contains an installation guide, a quick tour of Mangrove's features, detailed chapters about each of the features, and a Doxygen-generated API reference. The documentation must be viewed on a static web server, even when viewing locally. If you have Node.js intalled, you can use the [`http-server`](https://www.npmjs.com/package/http-server) package to host the docs locally:

```
./generate_docs.sh
npm install -g http-server
http-server ./docs
```

If you'd like to run the docs in such a way that the page will refresh when the content is updated, you can run `hugo serve` in the `hugo` directory:

```
cd hugo
hugo serve
```

Note that the Doxygen API will be unavailable in this served documentation.

## Bugs and Issues

See our [JIRA project](http://jira.mongodb.org/browse/CXXODM).

## Mailing Lists and IRC

The [mongodb-user group](https://groups.google.com/forum/#!forum/mongodb-user) is the main forum for MongoDB technical questions.

Other community resources are outlined on the [MongoDB Community site](http://dochub.mongodb.org/core/community).

## License

The source files in this repository are made available under the terms of the Apache License, version 2.0.

The BSON mapper in Mangrove (Boson) is powered by the [Cereal](http://uscilab.github.io/cereal/) C++11 serialization library, which is made available under the BSD 3-Clause License.

The documentation for Mangrove utilizes the Hugo [Learn](https://github.com/matcornic/hugo-theme-learn) theme (Copyright (c) 2014 Grav Copyright (c) 2016 MATHIEU CORNIC) which is made available under the MIT License.


