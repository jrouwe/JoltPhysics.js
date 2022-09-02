# JoltPhysics.js

This project enables using Jolt Physics in JavaScript.

[Click here for a demo](http://htmlpreview.github.io/?https://github.com/jrouwe/JoltPhysics.js/blob/main/Examples/falling_shapes.html).

This project is work in progress, much of the interface hasn't been exposed yet. If you need something, just add it to JoltJS.idl and JoltJS.h and send a pull request.

## Using

A precompiled version of the library is here: [examples/js/ammo.js](examples/js/ammo.js). See [examples/falling_shapes.html](examples/falling_shapes.html) for an example on how to use the library.

## Building

This project has only been compiled under Linux.

* Install [emscripten](https://emscripten.org/) and ensure that its environment variables have been setup
* Install [cmake](https://cmake.org/)
* Ensure you clone this project with its submodules: `git clone --recursive https://github.com/jrouwe/JoltPhysics.js.git`
* Run ./build.sh

## Credits

This project was started from the [Ammo.js](https://github.com/kripken/ammo.js) code, but little remains of it as the Jolt Physics API is very different from the Bullet API.

## License

The project is distributed under the [MIT license](LICENSE).
