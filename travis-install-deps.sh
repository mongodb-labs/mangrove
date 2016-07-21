#!/bin/sh

set -e;

if [ ! -d "$HOME/deps/lib" ]; then
	# Make the deps directory
	mkdir -p $HOME/deps;

	# Download and Install Mongo C Driver
	curl -O -L https://github.com/mongodb/mongo-c-driver/releases/download/${CDRIVER_VERSION}/mongo-c-driver-${CDRIVER_VERSION}.tar.gz
	tar -zxf mongo-c-driver-${CDRIVER_VERSION}.tar.gz
	pushd mongo-c-driver-${CDRIVER_VERSION}; 
	./configure --prefix=$HOME/deps && make -j$(grep -c ^processor /proc/cpuinfo) && make install;
	popd;

	# Enter deps directory
	pushd $HOME/deps;

	# Download and Install Mongo C++ Driver
	git clone -b 'master' --single-branch https://github.com/mongodb/mongo-cxx-driver.git;
	pushd mongo-cxx-driver/build;
	$HOME/build/mongodb/mongo-cxx-odm/build/${CMAKE_BINARY} -DCMAKE_CXX_FLAGS="-Wno-ignored-attributes" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$HOME/deps/ .. && make -j$(grep -c ^processor /proc/cpuinfo) && make install;
	popd;

	# Leave deps directory
	popd;
else
	echo 'Using cached directory for dependencies.';
fi
