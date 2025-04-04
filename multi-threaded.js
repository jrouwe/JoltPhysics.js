// START: MULTI THREADED
if (ENVIRONMENT_IS_PTHREAD) {
	const _invokeEntryPoint = invokeEntryPoint;
	invokeEntryPoint = (ptr, arg) => {
		if (arg.script) {
			// Workaround for bug: https://github.com/jrouwe/JoltPhysics.js/issues/245
			// `replace_by_import` gets replaced by `import`
			replace_by_import(arg.script).then(module => {
				module.default(Module, arg.params).then(() => _invokeEntryPoint(ptr, arg.value));
			})
		} else {
			_invokeEntryPoint(ptr, arg.value);
		}
	}
} else {
	const _spawnThread = spawnThread;
	spawnThread = (threadParams) => {
		threadParams.arg = { value: threadParams.arg, script: Module['workerScript'], params: Module['workerScriptParams'] };
		_spawnThread(threadParams);
	}
	Module['configureWorkerScripts'] = (scriptUrl, scriptParams) => {
		Module['workerScript'] = scriptUrl;
		Module['workerScriptParams'] = scriptParams;
	}
}
// END: MULTI THREADED