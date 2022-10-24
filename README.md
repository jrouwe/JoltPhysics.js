# JoltPhysics.js

This project enables using [Jolt Physics](https://github.com/jrouwe/JoltPhysics) in JavaScript.

## Demos

* [Falling Shapes Demo - Shows supported shape types](http://htmlpreview.github.io/?https://github.com/jrouwe/JoltPhysics.js/blob/main/Examples/falling_shapes.html).
* [Constraints Demo - Shows supported constraint types](http://htmlpreview.github.io/?https://github.com/jrouwe/JoltPhysics.js/blob/main/Examples/constraints.html).
* [Stress Test Demo - Shows big pile of blocks](http://htmlpreview.github.io/?https://github.com/jrouwe/JoltPhysics.js/blob/main/Examples/stress_test.html).

## Using

A precompiled version of the library is [here](Examples/js/jolt.js). See [falling_shapes.html](Examples/falling_shapes.html) for an example on how to use the library.

Not all of the Jolt interface has been exposed yet. If you need something, just add it to JoltJS.idl and JoltJS.h and send a pull request.

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
