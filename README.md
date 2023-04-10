# JoltPhysics.js

This project enables using [Jolt Physics](https://github.com/jrouwe/JoltPhysics) in JavaScript.

## Demos

* [Falling Shapes Demo - Shows supported shape types](https://jrouwe.nl/jolt/examples/falling_shapes.html).
* [Constraints Demo - Shows supported constraint types](https://jrouwe.nl/jolt/examples/constraints.html).
* [Stress Test Demo - Shows big pile of blocks](https://jrouwe.nl/jolt/examples/stress_test.html).

## Using

The library comes in 2 flavours: [a JavaScript version](https://jrouwe.nl/jolt/examples/js/jolt.js) and [a WASM version](https://jrouwe.nl/jolt/examples/js/jolt.wasm.js). See [falling_shapes.html](Examples/falling_shapes.html) for an example on how to use the library.

Not all of the Jolt interface has been exposed yet. If you need something, just add it to JoltJS.idl and JoltJS.h and send a pull request.

## Building

This project has only been compiled under Linux.

* Install [emscripten](https://emscripten.org/) and ensure that its environment variables have been setup
* Install [cmake](https://cmake.org/)
* Run ```./build.sh``` for the optimized build or ```./build.sh Debug``` for the debug build

## Running

By default the examples use the WASM version of Jolt. This requires serving the html file using a web server rather than opening the html file directly. Use e.g. [serve](https://www.npmjs.com/package/serve) to quickly host the file.

If you need to debug the C++ code take a look at [WASM debugging](https://developer.chrome.com/blog/wasm-debugging-2020/).

## Credits

This project was started from the [Ammo.js](https://github.com/kripken/ammo.js) code, but little remains of it as the Jolt Physics API is very different from the Bullet API.

## License

The project is distributed under the [MIT license](LICENSE).
