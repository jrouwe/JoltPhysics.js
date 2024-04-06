import initJolt from 'jolt-physics';

initJolt().then(function (Jolt) {
	// Record how much memory we have in the beginning
	const memoryFreeBefore = Jolt.JoltInterface.prototype.sGetFreeMemory();

	// Create very simple object layer filter with only a single layer
	const MY_LAYER = 0;
	let objectFilter = new Jolt.ObjectLayerPairFilterTable(1);
	objectFilter.EnableCollision(MY_LAYER, MY_LAYER);

	// Create very simple broad phase layer interface with only a single layer
	const BP_LAYER = new Jolt.BroadPhaseLayer(0);
	let bpInterface = new Jolt.BroadPhaseLayerInterfaceTable(1, 1);
	bpInterface.MapObjectToBroadPhaseLayer(MY_LAYER, BP_LAYER);
	Jolt.destroy(BP_LAYER); // 'BP_LAYER' has been copied into bpInterface

	// Create broad phase filter
	let bpFilter = new Jolt.ObjectVsBroadPhaseLayerFilterTable(bpInterface, 1, objectFilter, 1);

	// Initialize Jolt
	let settings = new Jolt.JoltSettings();
	settings.mObjectLayerPairFilter = objectFilter;
	settings.mBroadPhaseLayerInterface = bpInterface;
	settings.mObjectVsBroadPhaseLayerFilter = bpFilter;
	let jolt = new Jolt.JoltInterface(settings); // Everything in 'settings' has now been copied into 'jolt', the 3 interfaces above are now owned by 'jolt'
	Jolt.destroy(settings);

	// Typing shortcuts
	let physicsSystem = jolt.GetPhysicsSystem();
	let bodyInterface = physicsSystem.GetBodyInterface();

	// Create a box
	let material = new Jolt.PhysicsMaterial();
	let size = new Jolt.Vec3(4, 0.5, 0.5);
	let box = new Jolt.BoxShapeSettings(size, 0.05, material); // 'material' is now owned by 'box'
	Jolt.destroy(size);

	// Create a compound
	let compound = new Jolt.StaticCompoundShapeSettings();
	let boxPosition = new Jolt.Vec3(5, 0, 0);
	compound.AddShape(boxPosition, Jolt.Quat.prototype.sIdentity(), box); // 'box' is now owned by 'compound'
	Jolt.destroy(boxPosition);
	let shapeResult = compound.Create();
	let shape = shapeResult.Get();
	shapeResult.Clear(); // We no longer need the shape result, it keeps a reference to 'shape' (which it will also release the next time you create another shape)
	shape.AddRef(); // We want to own this shape so we can delete 'compound' which internally keeps a reference
	Jolt.destroy(compound);

	// Create the body
	let bodyPosition = new Jolt.RVec3(1, 2, 3);
	let bodyRotation = new Jolt.Quat(0, 0, 0, 1);
	let creationSettings = new Jolt.BodyCreationSettings(shape, bodyPosition, bodyRotation, Jolt.EMotionType_Dynamic, MY_LAYER); // 'creationSettings' now holds a reference to 'shape'
	Jolt.destroy(bodyPosition);
	Jolt.destroy(bodyRotation);
	shape.Release(); // We no longer need our own reference to 'shape' because 'creationSettings' now has one
	let body = bodyInterface.CreateBody(creationSettings);
	Jolt.destroy(creationSettings); // 'creationSettings' no longer needed, all settings and the shape reference went to 'body'

	// Add the body
	bodyInterface.AddBody(body.GetID(), Jolt.EActivation_Activate);

	// Log result
	let oldPosition = body.GetPosition();
	console.log("Before the step the body is at ", oldPosition.GetX(), oldPosition.GetY(), oldPosition.GetZ());

	// Step the world
	jolt.Step(1.0 / 60.0, 1);

	// Log result
	let newPosition = body.GetPosition();
	console.log("After 1 step the body is now at ", newPosition.GetX(), newPosition.GetY(), newPosition.GetZ());

	// Remove and destroy the body
	bodyInterface.RemoveBody(body.GetID());
	bodyInterface.DestroyBody(body.GetID()); // This will internally do the equivalent of Jolt.destroy(body)

	// Clean up the jolt interface
	Jolt.destroy(jolt);

	// Record how much memory we have at the end
	console.log("Memory leaked: " + (memoryFreeBefore - Jolt.JoltInterface.prototype.sGetFreeMemory()) + " bytes");
});