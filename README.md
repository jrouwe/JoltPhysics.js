[![Version](https://img.shields.io/npm/v/jolt-physics)](https://www.npmjs.com/package/jolt-physics)
[![Downloads](https://img.shields.io/npm/dt/jolt-physics.svg)](https://www.npmjs.com/package/jolt-physics)
[![Bundle Size](https://img.shields.io/bundlephobia/min/jolt-physics?label=bundle%20size)](https://bundlephobia.com/result?p=jolt-physics)
[![Build Status](https://github.com/jrouwe/JoltPhysics.js/actions/workflows/build-and-deploy.yml/badge.svg)](https://github.com/jrouwe/JoltPhysics.js/actions/)

# JoltPhysics.js

This project enables using [Jolt Physics](https://github.com/jrouwe/JoltPhysics) in JavaScript.

## Demos

Go to the [demos page](https://jrouwe.github.io/JoltPhysics.js/) to see the project in action.

## Using

This library comes in 3 flavours:
- `wasm-compat` - A WASM version with the WASM file (encoded in base64) embedded in the bundle
- `wasm` - A WASM version with a separate WASM file
- `asm` - A JavaScript version that uses [asm.js](https://developer.mozilla.org/en-US/docs/Games/Tools/asm.js)

See [falling_shapes.html](Examples/falling_shapes.html) for a example on how to use the library.

### Documentation

The interface of the library is the same as the C++ interface of JoltPhysics, this means that you can use the [C++ documentation](https://jrouwe.github.io/JoltPhysics/) as reference.

Not all of the Jolt interface has been exposed yet. Check [JoltJS.idl](https://github.com/jrouwe/JoltPhysics.js/blob/main/JoltJS.idl) if a particular interface has been exposed. If not, edit [JoltJS.idl](https://github.com/jrouwe/JoltPhysics.js/blob/main/JoltJS.idl) and [JoltJS.h](https://github.com/jrouwe/JoltPhysics.js/blob/main/JoltJS.h) and send a pull request, or open an issue.

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
* Run ```./build.sh``` for the optimized build or ```./build.sh Debug``` for the debug build

## Running

By default the examples use the WASM version of Jolt. This requires serving the html file using a web server rather than opening the html file directly. Use e.g. [serve](https://www.npmjs.com/package/serve) to quickly host the file.

If you need to debug the C++ code take a look at [WASM debugging](https://developer.chrome.com/blog/wasm-debugging-2020/).

## Credits

This project was started from the [Ammo.js](https://github.com/kripken/ammo.js) code, but little remains of it as the Jolt Physics API is very different from the Bullet API.

## License

The project is distributed under the [MIT license](LICENSE).
