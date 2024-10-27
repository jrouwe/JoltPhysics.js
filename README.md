[![Version](https://img.shields.io/npm/v/jolt-physics)](https://www.npmjs.com/package/jolt-physics)
[![Downloads](https://img.shields.io/npm/dt/jolt-physics.svg)](https://www.npmjs.com/package/jolt-physics)
[![Bundle Size](https://img.shields.io/bundlephobia/min/jolt-physics?label=bundle%20size)](https://bundlephobia.com/result?p=jolt-physics)
[![Build Status](https://github.com/jrouwe/JoltPhysics.js/actions/workflows/build-and-deploy.yml/badge.svg)](https://github.com/jrouwe/JoltPhysics.js/actions/)

# JoltPhysics.js

This project enables using [Jolt Physics](https://github.com/jrouwe/JoltPhysics) in JavaScript.

## Demos

Go to the [demos page](https://jrouwe.github.io/JoltPhysics.js/) to see the project in action.

## Using

This library comes in 5 flavours:
- `wasm-compat` - A WASM version with the WASM file (encoded in base64) embedded in the bundle
- `wasm` - A WASM version with a separate WASM file
- `asm` - A JavaScript version that uses [asm.js](https://developer.mozilla.org/en-US/docs/Games/Tools/asm.js)
- `wasm-compat-multithread` - Same as `wasm-compat` but with multi threading enabled.
- `wasm-multithread` - Same as `wasm` but with multi threading enabled.

See [falling_shapes.html](Examples/falling_shapes.html) for an example on how to use the library.

### Documentation

The interface of the library is the same as the C++ interface of JoltPhysics, this means that you can use the [C++ documentation](https://jrouwe.github.io/JoltPhysics/) as reference.

Almost the entire Jolt interface has been exposed. Check [JoltJS.idl](https://github.com/jrouwe/JoltPhysics.js/blob/main/JoltJS.idl) if a particular interface has been exposed. If not, edit [JoltJS.idl](https://github.com/jrouwe/JoltPhysics.js/blob/main/JoltJS.idl) and [JoltJS.h](https://github.com/jrouwe/JoltPhysics.js/blob/main/JoltJS.h) and send a pull request, or open an issue.

### Installation

This library is distributed as ECMAScript modules on npm:

```sh
npm install jolt-physics
```

The different flavours are available via entrypoints on the npm package:

```js
// WASM embedded in the bundle
import Jolt from 'jolt-physics';
import Jolt from 'jolt-physics/wasm-compat';

// WASM
import Jolt from 'jolt-physics/wasm';

// asm.js
import Jolt from 'jolt-physics/asm';

// WASM embedded in the bundle, multithread enabled
import Jolt from 'jolt-physics/wasm-compat-multithread';

// WASM, multithread enabled
import Jolt from 'jolt-physics/wasm-multithread';
```

You can also import esm bundles with unpkg:

```html
<script type="module">
    // import latest
    import Jolt from 'https://www.unpkg.com/jolt-physics/dist/jolt-physics.wasm-compat.js';

    // or import a specific version
    import Jolt from 'https://www.unpkg.com/jolt-physics@x.y.z/dist/jolt-physics.wasm-compat.js';
</script>
```

Where ```x.y.z``` is the version of the library you want to use.

### Using the WASM flavour

To use the `wasm` flavour, you must either serve the WASM file `jolt-physics.wasm.wasm` alongside `jolt-physics.wasm.js`, or use a bundler that supports importing an asset as a url, and tell Jolt where to find the WASM file.

To specify where to retrieve the WASM file from, you can pass a `locateFile` function to the default export of `jolt-physics/wasm`. For example, using [vite](https://vitejs.dev/) this would look like: 

```js
import initJolt from "jolt-physics";
import joltWasmUrl from "jolt-physics/jolt-physics.wasm.wasm?url";

const Jolt = await initJolt({
  locateFile: () => joltWasmUrl,
});
```

For more information on the `locateFile` function, see the [Emscripten documentation](https://emscripten.org/docs/api_reference/module.html#Module.locateFile).

## Building

This project has only been compiled under Linux.

* Install [emscripten](https://emscripten.org/) and ensure that its environment variables have been setup
* Install [cmake](https://cmake.org/)
* Run ```./build.sh Distribution``` for the optimized build, ```./build.sh Debug``` for the debug build.

Additional options that can be provided to ```build.sh```:

* ```-DENABLE_MEMORY_PROFILER=ON``` will enable memory tracking to detect leaks.
* ```-DDOUBLE_PRECISION=ON``` will enable the double precision mode. This allows worlds larger than a couple of km.
* ```-DENABLE_SIMD=ON``` will enable SIMD instructions. Safari 16.4 was the last major browser to support this (in March 2023).
* ```-DBUILD_WASM_COMPAT_ONLY=ON``` speeds up the build by only compiling the WASM compat version which the examples use.

## Running

By default the examples use the WASM compat version of Jolt. This requires serving the html file using a web server rather than opening the html file directly.

Open a terminal in this folder and run the following commands:

```
npm install
npm run examples
```

Then navigate to: http://localhost:3000/

If you need to debug the C++ code take a look at [WASM debugging](https://developer.chrome.com/blog/wasm-debugging-2020/).

## Memory Management

The samples are very bad at cleaning up after themselves (basically they don't). When using emscripten to port a library to WASM, [nothing is cleaned up](https://emscripten.org/docs/porting/connecting_cpp_and_javascript/WebIDL-Binder.html#using-c-classes-in-javascript) automatically, so everything you newed with ```new Jolt.XXX``` needs to be destroyed by ```Jolt.destroy(...)```.

On top of this, Jolt uses reference counting for a number of its classes (everything that inherits from [RefTarget](https://jrouwe.github.io/JoltPhysics/class_ref_target.html)). The most important classes are:

* ShapeSettings
* Shape
* ConstraintSettings
* Constraint
* PhysicsMaterial
* GroupFilter
* SoftBodySharedSettings
* VehicleCollisionTester
* VehicleController
* WheelSettings
* CharacterBaseSettings
* CharacterBase

Reference counting objects start with a reference count of 0. If you want to keep ownership over the object, you need to call ```object.AddRef()```, this will increment the reference count. If you want to release ownership you call ```object.Release()```, this will decrement the reference count and if the reference count reaches 0 the object will be destroyed. If, after newing, you pass a reference counted object on to another object (e.g. a ShapeSettings to a CompoundShapeSettings or a Shape to a Body) then that other object will take a reference, in that case it is not needed take a reference yourself beforehand so you can skip the calls to ```AddRef/Release```. Note that it is also possible to do ```new Jolt.XXX``` followed by ```Jolt.destroy(...)``` for a reference counted object if no one took a reference.

The Body class is also a special case, it is destroyed through BodyInterface.DestroyBody(body.GetID()) (which internally destroys the Body).

Almost everything else can be destroyed straight after it has been passed to Jolt. [An example that shows how to properly clean up using Jolt is here](https://github.com/jrouwe/JoltPhysics.js/blob/main/Examples/proper_cleanup.html).

## Projects using JoltPhysics.js

* [Babylon.js plugin](https://github.com/PhoenixIllusion/babylonjs-jolt-physics-plugin) - A plugin that replaces the default physics engine with Jolt.
* [react-three-jolt](https://github.com/pmndrs/react-three-jolt) - Wraps Jolt to make it easy to use in react-three-fiber.
* [r3f-jolt](https://github.com/sajal353/r3f-jolt) - Another wrapper for react-three-fiber.

## License

The project is distributed under the [MIT license](LICENSE).
