
# Installation

The official Latitude virtual machine is supported on POSIX-compliant
machines and on recent versions of Windows.

To build Latitude from source, you will need the following.
 * A C++14-compliant compiler
 * A recent version of the Boost C++ libraries
 * Perl 5.010 or newer
 * GNU Flex and Bison

## For POSIX-Compliant Operating Systems

Run the following command, from the base directory.

    $ make release

If your compiler goes by a different name, you may have to pass
`CXX=compiler_name` and `CC=compiler_name`. If the C++ Boost libraries
are in a location that is not included by default, you may include
`BOOST=-I /path/to/boost/` in the build command.

Building the `release` target is recommended for maximal performance.
In order to set Latitude up so that you can run it from the command
line, run

    $ sudo make install

If you do not have `sudo` or do not wish to install Latitude to
`/usr/local/bin`, you may add the base project directory to your
`PATH` environment variable manually.

After installation, you may run the Latitude interpreter with

    $ latitude

or you may run a Latitude script file with

    $ latitude filename.lats

## For Windows

Run the following command, from the base directory.

    $ make release

If your compiler goes by a different name, you may have to pass
`CXX=compiler_name` and `CC=compiler_name`. If the C++ Boost libraries
are in a location that is not included by default, you may include
`BOOST=-I /path/to/boost/` in the build command.

Building the `release` target is recommended for maximal performance.
Latitude is tested on Windows with MinGW, so it is recommended that it
be built with MinGW on Windows.

After building, add the base project directory to your `PATH`
environment variable, and then you can invoke the Latitude interpreter
with

    $ latitude

or you may run a Latitude script file with

    $ latitude filename.lats

## For Other Operating Systems

This Latitude virtual machine is not officially supported on any
operating systems other than those listed above. However, if you wish
to attempt to build it, here are some steps to set you in the right
direction.

First, you will need to teach the VM how to detect your OS. Visit
`Platform.hpp` and add a preprocessor conditional. This conditional
should test against some preprocessor macro which should exist on your
platform and, in the case that the test is satisfied, should define a
macro of the form `USE_*`, where `*` is the name of your OS. The
placement of this test should ensure that, in the case that your OS is
identified, `USE_NULL` will *not* be defined.

Next, in the following files, there is OS-specific code protected by
preprocessor conditions. You should add cases for your OS, which
delegate to the appropriate system-level calls.
 * `Pathname.cpp`
 * `Platform.hpp`
 * `Process.cpp`
 * `Environment.cpp`
 * `Input.cpp`

Provided that you have a working, up-to-date C++ compiler and the
necessary dependencies, Latitude should now build. If you can
successfully get Latitude to build on a non-supported OS, please
consider making a pull request to the project on Github, as your
changes may be incorporated officially.

[[up](..)]
