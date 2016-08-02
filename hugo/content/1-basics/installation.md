+++
next = "/1-basics/quick-tour"
prev = "/1-basics/what-is-mangrove"
title = "Installation"
toc = true
weight = 2

+++

To get Mangrove up and running in your project, simply follow these instructions to install Mangrove on your system.

## Prerequisites

1. Any little-endian platform.
2. A modern compiler that supports C++14.
	* Mangrove has been confirmed to work with Clang 3.8+, Apple Clang 7.3+, or GCC 6.1+.
3. CMake 3.2+. 
4. The MongoDB C driver version 1.3.5+. (see below)
5. The MongoDB C++ driver version 3.0.2+. (see below)
6. *(Optional)* pkg-config

{{% notice note %}}
As of this writing, the MongoDB C++ driver version 3.0.2 has not been released. When installing the C++ driver, simply `git checkout master` if `git checkout r3.0.2` does not work.
{{% /notice %}}

## Build and install the C driver

Mangrove uses {{% a_blank "libbson" "https://api.mongodb.com/libbson/current/" %}} and {{% a_blank "libmongoc" "https://api.mongodb.com/c/current/" %}} internally. If you don't already have a new enough version of libmongoc and libbson installed, then you need to build them.

Build and install libmongoc according to the section {{% a_blank "Building From a Release Tarball" "https://api.mongodb.com/c/current/installing.html#unix-build" %}} in the install instructions. libmongoc installs libbson if necessary.

## Build and install the C++11 driver

Mangrove also uses the {{% a_blank "MongoDB C++11 Driver" "https://github.com/mongodb/mongo-cxx-driver" %}} internally.

Build and install the C++ driver according to its {{% a_blank "quickstart guide" "https://github.com/mongodb/mongo-cxx-driver/wiki/Quickstart-Guide-(New-Driver)" %}}.

## Build and install Mangrove

Clone the repository, and check out the latest stable release.

* `git clone -b master https://github.com/mongodb-labs/mangrove`

Build Mangrove with the following commands:

* `cd mangrove/build`
* `[PKG_CONFIG_PATH=CXXDRIVER_INSTALL_PATH/lib/pkgconfig] cmake -DCMAKE_BUILD_TYPE=Release [-DCMAKE_INSTALL_PREFIX=DESIRED_INSTALL_PATH] ..`
* `make && sudo make install`

{{% notice note %}}
Note that if you installed the C driver and C++ driver to a path that is automatically searched by `pkg-config`, you can omit the `PKG_CONFIG_PATH` environment variable. If you don't have `pkg-config`, you can explicitly set the path to the libbson, libmongoc, libbsoncxx, and libmongocxx install prefixes with the `-DLIBBSON_DIR`, `-DLIBMONGOC_DIR`, -`Dlibbsoncxx_DIR`, and `-Dlibmongocxx_DIR` CMake arguments.
{{% /notice %}}

## Using Mangrove

Once you've installed Mangrove, you can start using it by including and linking `libmangrove`, `libboson`, and `libmongocxx` in your project. `libmangrove` is the library that contains the actual ODM, `libboson` is the BSON serialization library that powers `libmangrove`, and `libmongocxx` is the MongoDB C++ Driver.

If you're using `pkg-config`, using Mangrove is incredibly simple. Simply add the following two flags to your compiler invocation:

* `--std=c++14`
* `$(pkg-config --libs --cflags libmangrove libboson libmongocxx)`
	- You may need to preface this with `PKG_CONFIG_PATH=MANGROVE_INSTALL_PATH/lib/pkgconfig` if you installed Mangrove in a directory not automatically searched for by pkg-config.

If you're not using `pkg-config`, you'll need to add the following flags to your compiler invocation:

* `--std=c++14`
* `-IMANGROVE_INSTALL_PATH/include/mangrove/v_noabi` 
* `-IMANGROVE_INSTALL_PATH/include/boson/v_noabi`
* `-IMANGROVE_INSTALL_PATH/include/boson/v_noabi/boson/third_party`
* `-ICXXDRIVER_INSTALL_PATH/include/mongocxx/v_noabi`
* `-ICDRIVER_INSTALL_PATH/include/libmongoc-1.0`
* `-ICXXDRIVER_INSTALL_PATH/include/bsoncxx/v_noabi`
* `-ICDRIVER_INSTALL_PATH/include/libbson-1.0` 
* `-LMANGROVE_INSTALL_PATH/lib`
	- If the install prefixes for the C and C++ Drivers are different than the one for Mangrove, also include `-LCDRIVER_INSTALL_PATH/lib` and `-LCXX_DRIVER_INSTALL_PATH/lib`
* `-lmangrove` 
* `-lboson`
* `-lmongocxx` 
* `-lbsoncxx`
