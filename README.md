# a-g
A package manager that compiles source code using a simple set of build commands which is customizable.

## Build

#### 1. Clone Source
    git clone https://github.com/CoderRC/a-g.git
    cd a-g

#### 2. Build
    mkdir build
    cd build
    ../configure
    make

## Install
    make install

## Package manager command arguments

#### Install
The install command argument installs the application from compiled source code using a simple set of build commands which is customizable.

For example:

    a-g install a-g

Installs a-g from compiled source code using a simple set of build commands which is customizable.

To install a-g using a-g in mingw first to gain posix c functions create configure.conf in etc/a-g/default/config relative to a-g executable with

    LDFLAGS=-lmingw32_extended

#### Update
The update command argument fetches all installed application's source code.

For example:

    a-g update

Fetches all installed application's source code.

#### Upgrade
The upgrade command argument upgrades the application from compiled source code using a simple set of build commands which is customizable.

For example:

    a-g upgrade

Upgrades all applications that have a different commit hash than the installed application's commit hash which is changed by the update command argument from compiled source code using a simple set of build commands which is customizable.

#### Remove
The remove command argument removes all specified installed applications.

For example:

    a-g remove a-g

Removes a-g installed application.

## Package manager's etc directory
This directory changes a-g settings.

#### sources.list

This file stores all the websites that contain the applications source code.

#### default

This directory stores the default settings for compiling source code.

##### config

This directory stores the configuration of a specified command for compiling source code.

For example create configure.conf in etc/a-g/default/config relative to a-g executable with

    LDFLAGS=-lmingw32_extended

So that gcc in mingw can gain posix c functions that will be used in all of the non override settings for compiling source code.

##### run

This directory stores the build commands that is used for compiling source code.

##### runconfig

This directory stores the location of the specified command line configuration for compiling source code.

#### additional

This directory stores the additional settings for compiling source code in for a specified application with name as a subdirectory.

For example etc/a-g/default/config and etc/a-g/additional/a-g/config if want to add additional configuration of a specified command for compiling source code to a-g installed application.

#### override

This directory overrides default settings for compiling source code in for a specified application with name as a subdirectory.

For example etc/a-g/default/config and etc/a-g/override/a-g/config if want to override default configuration of a specified command for compiling source code to a-g installed application.

## Requirements

Download a sh command line environment to run configure.

Download git to use the git command for cloning the source.

Download make to compile the library.

Download gcc with posix c functions to compile the source and configure it.

If the gcc is mingw and the sh command line environment supports the pacman command do

    pacman -S git
    pacman -S make
    pacman -S mingw-w64-x86_64-gcc
    git clone https://github.com/CoderRC/libmingw32_extended.git
    cd libmingw32_extended
    mkdir build
    cd build
    ../configure
    make
    make install
    cd ../..
    git clone https://github.com/CoderRC/a-g.git
    cd a-g
    mkdir build
    cd build
    ../configure LDFLAGS=-lmingw32_extended
    make

If the sh command line environment supports the pacman command do

    pacman -S git
    pacman -S make
    pacman -S gcc

## Contributing to the source

To properly add new sources to the repository, the sources must be added to the source directory in the repository.
