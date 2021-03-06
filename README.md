Drawpile - a collaborative drawing program
------------------------------------------

DrawPile is a drawing program with a twist: you can share your drawing
live with other people.

Some feature highlights:

* Shared canvas using the built-in server or a dedicated server
* Record, play back and export drawing sessions
* Layers and blending modes
* Text layers
* Supports pressure sensitive Wacom tablets
* Built-in chat
* Supports OpenRaster file format

## Building with cmake

Dependencies:

* Qt 5.0 or newer
* zlib

For building just the dedicated server, only QtCore and QtNetwork are required.

It's a good idea to build in a separate directory to keep build files
separate from the source tree.

Example:

    $ mkdir build
    $ cd build
    $ cmake ..
    $ make

The executables will be generated in the `build/bin` directory. You can run them from there,
or install them with `make install`.

The configuration step supports some options:

* CLIENT=off: don't build the client (useful when building the stand-alone server only)
* SERVER=off: don't build the stand-alone server.
* DEBUG=on: enable debugging features

Example: `$ cmake .. -DDEBUG=on`

