// Graphics variables
var container, stats;
var camera, controls, scene, renderer;

// Timers
var clock = new THREE.Clock();
var time = 0;

// Physics variables
var jolt;
var physicsSystem;
var bodyInterface;

// List of objects spawned
var dynamicObjects = [];

// The update function
var onExampleUpdate;

const DegreesToRadians = (deg) => deg * (Math.PI / 180.0);

const wrapVec3 = (v) => new THREE.Vector3(v.GetX(), v.GetY(), v.GetZ());
const unwrapVec3 = (v) => new Jolt.Vec3(v.x, v.y, v.z);
const wrapQuat = (q) => new THREE.Quaternion(q.GetX(), q.GetY(), q.GetZ(), q.GetW());
const unwrapQuat = (q) => new Jolt.Quat(q.x, q.y, q.z, q.w);

// Object layers
const LAYER_NON_MOVING = 0;
const LAYER_MOVING = 1;

function getRandomQuat() {
	return Jolt.Quat.prototype.sRotation(new Jolt.Vec3(0.001 + Math.random(), Math.random(), Math.random()).Normalized(), 2 * Math.PI * Math.random());
}

function onWindowResize() {

	camera.aspect = window.innerWidth / window.innerHeight;
	camera.updateProjectionMatrix();

	renderer.setSize(window.innerWidth, window.innerHeight);
}

function initGraphics() {

	renderer = new THREE.WebGLRenderer();
	renderer.setClearColor(0xbfd1e5);
	renderer.setPixelRatio(window.devicePixelRatio);
	renderer.setSize(window.innerWidth, window.innerHeight);

	camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.2, 2000);
	camera.position.set(0, 15, 30);
	camera.lookAt(new THREE.Vector3(0, 0, 0));

	scene = new THREE.Scene();

	var dirLight = new THREE.DirectionalLight(0xffffff, 1);
	dirLight.position.set(10, 10, 5);
	scene.add(dirLight);

	controls = new THREE.OrbitControls(camera, container);

	container.appendChild(renderer.domElement);

	stats = new Stats();
	stats.domElement.style.position = 'absolute';
	stats.domElement.style.top = '0px';
	container.appendChild(stats.domElement);

	window.addEventListener('resize', onWindowResize, false);
}

function initPhysics() {
	// We use only 2 layers: one for non-moving objects and one for moving objects
	let objectFilter = new Jolt.ObjectLayerPairFilterTable(2);
	objectFilter.EnableCollision(LAYER_NON_MOVING, LAYER_MOVING);
	objectFilter.EnableCollision(LAYER_MOVING, LAYER_MOVING);

	// We use a 1-to-1 mapping between object layers and broadphase layers
	const BP_LAYER_NON_MOVING = new Jolt.BroadPhaseLayer(0);
	const BP_LAYER_MOVING = new Jolt.BroadPhaseLayer(1);
	let bpInterface = new Jolt.BroadPhaseLayerInterfaceTable(2, 2);
	bpInterface.MapObjectToBroadPhaseLayer(LAYER_NON_MOVING, BP_LAYER_NON_MOVING);
	bpInterface.MapObjectToBroadPhaseLayer(LAYER_MOVING, BP_LAYER_MOVING);

	// Initialize Jolt
	settings = new Jolt.JoltSettings();
	settings.mObjectLayerPairFilter = objectFilter;
	settings.mBroadPhaseLayerInterface = bpInterface;
	settings.mObjectVsBroadPhaseLayerFilter = new Jolt.ObjectVsBroadPhaseLayerFilterTable(settings.mBroadPhaseLayerInterface, 2, settings.mObjectLayerPairFilter, 2);
	jolt = new Jolt.JoltInterface(settings);
	physicsSystem = jolt.GetPhysicsSystem();
	bodyInterface = physicsSystem.GetBodyInterface();

	// Helper functions
	Jolt.Vec3.prototype.ToString = function () { return `(${this.GetX()}, ${this.GetY()}, ${this.GetZ()})` };
	Jolt.Vec3.prototype.Clone = function () { return new Jolt.Vec3(this.GetX(), this.GetY(), this.GetZ()); };
	Jolt.Quat.prototype.ToString = function () { return `(${this.GetX()}, ${this.GetY()}, ${this.GetZ()}, ${this.GetW()})` };
	Jolt.Quat.prototype.Clone = function () { return new Jolt.Vec3(this.GetX(), this.GetY(), this.GetZ(), this.GetW()); };
	Jolt.AABox.prototype.ToString = function () { return `[${this.mMax.ToString()}, ${this.mMin.ToString()}]`; };
}

function updatePhysics(deltaTime) {

	// When running below 55 Hz, do 2 steps instead of 1
	var numSteps = deltaTime > 1.0 / 55.0 ? 2 : 1;

	// Step the physics world
	jolt.Step(deltaTime, numSteps);
}

function initExample(Jolt, updateFunction) {
	window.Jolt = Jolt;

	container = document.getElementById('container');
	container.innerHTML = "";

	if (WebGL.isWebGLAvailable()) {
		onExampleUpdate = updateFunction;

		initGraphics();
		initPhysics();
		renderExample();
	} else {
		const warning = WebGL.getWebGLErrorMessage();
		container.appendChild(warning);
	}
}

function renderExample() {

	requestAnimationFrame(renderExample);

	// Don't go below 30 Hz to prevent spiral of death
	var deltaTime = clock.getDelta();
	deltaTime = Math.min(deltaTime, 1.0 / 30.0);

	if (onExampleUpdate != null)
		onExampleUpdate(time, deltaTime);

	// Update object transforms
	for (let i = 0, il = dynamicObjects.length; i < il; i++) {
		let objThree = dynamicObjects[i];
		let body = objThree.userData.body;
		objThree.position.copy(wrapVec3(body.GetPosition()));
		objThree.quaternion.copy(wrapQuat(body.GetRotation()));

		if (body.GetBodyType() == Jolt.EBodyType_SoftBody)
			objThree.geometry = createMeshForShape(body.GetShape());
	}

	time += deltaTime;

	updatePhysics(deltaTime);

	controls.update(deltaTime);

	renderer.render(scene, camera);

	stats.update();
}

function addToScene(body, color) {
	bodyInterface.AddBody(body.GetID(), Jolt.EActivation_Activate);

	let threeObject = getThreeObjectForBody(body, color);
	threeObject.userData.body = body;

	scene.add(threeObject);
	dynamicObjects.push(threeObject);
}

function removeFromScene(threeObject) {
	let id = threeObject.userData.body.GetID();
	bodyInterface.RemoveBody(id);
	bodyInterface.DestroyBody(id);
	delete threeObject.userData.body;

	scene.remove(threeObject);
	let idx = dynamicObjects.indexOf(threeObject);
	dynamicObjects.splice(idx, 1);
}

function createFloor(size = 50) {
	var shape = new Jolt.BoxShape(new Jolt.Vec3(size, 0.5, size), 0.05, null);
	var creationSettings = new Jolt.BodyCreationSettings(shape, new Jolt.Vec3(0, -0.5, 0), new Jolt.Quat(0, 0, 0, 1), Jolt.EBodyType_Static, LAYER_NON_MOVING);
	let body = bodyInterface.CreateBody(creationSettings);
	addToScene(body, 0xc7c7c7);
	return body;
}

function createBox(position, rotation, halfExtent, motionType, layer, color = 0xffffff) {
	let shape = new Jolt.BoxShape(halfExtent, 0.05, null);
	let creationSettings = new Jolt.BodyCreationSettings(shape, position, rotation, motionType, layer);
	let body = bodyInterface.CreateBody(creationSettings);
	addToScene(body, color);
	return body;
}

function createSphere(position, radius, motionType, layer, color = 0xffffff) {
	let shape = new Jolt.SphereShape(radius, null);
	let creationSettings = new Jolt.BodyCreationSettings(shape, position, Jolt.Quat.prototype.sIdentity(), motionType, layer);
	let body = bodyInterface.CreateBody(creationSettings);
	addToScene(body, color);
	return body;
}

function createMeshForShape(shape) {
	// Get triangle data
	let triContext = new Jolt.ShapeGetTriangles(shape, Jolt.AABox.prototype.sBiggest(), shape.GetCenterOfMass(), Jolt.Quat.prototype.sIdentity(), new Jolt.Vec3(1, 1, 1));

	// Get a view on the triangle data (does not make a copy)
	let vertices = new Float32Array(Jolt.HEAPF32.buffer, triContext.GetVerticesData(), triContext.GetVerticesSize() / Float32Array.BYTES_PER_ELEMENT);

	// Now move the triangle data to a buffer and clone it so that we can free the memory from the C++ heap (which could be limited in size)
	let buffer = new THREE.BufferAttribute(vertices, 3).clone();
	Jolt.destroy(triContext);

	// Create a three mesh
	let geometry = new THREE.BufferGeometry();
	geometry.setAttribute('position', buffer);
	geometry.computeVertexNormals();

	return geometry;
}

function getThreeObjectForBody(body, color) {
	let material = new THREE.MeshPhongMaterial({ color: color });

	let threeObject;
	let shape = body.GetShape();
	switch (shape.GetSubType()) {
		case Jolt.EShapeSubType_Box:
			let boxShape = Jolt.castObject(shape, Jolt.BoxShape);
			let extent = wrapVec3(boxShape.GetHalfExtent()).multiplyScalar(2);
			threeObject = new THREE.Mesh(new THREE.BoxGeometry(extent.x, extent.y, extent.z, 1, 1, 1), material);
			break;
		case Jolt.EShapeSubType_Sphere:
			let sphereShape = Jolt.castObject(shape, Jolt.SphereShape);
			threeObject = new THREE.Mesh(new THREE.SphereGeometry(sphereShape.GetRadius(), 32, 32), material);
			break;
		case Jolt.EShapeSubType_Capsule:
			let capsuleShape = Jolt.castObject(shape, Jolt.CapsuleShape);
			threeObject = new THREE.Mesh(new THREE.CapsuleGeometry(capsuleShape.GetRadius(), 2 * capsuleShape.GetHalfHeightOfCylinder(), 20, 10), material);
			break;
		case Jolt.EShapeSubType_Cylinder:
			let cylinderShape = Jolt.castObject(shape, Jolt.CylinderShape);
			threeObject = new THREE.Mesh(new THREE.CylinderGeometry(cylinderShape.GetRadius(), cylinderShape.GetRadius(), 2 * cylinderShape.GetHalfHeight(), 20, 1), material);
			break;
		default:
			threeObject = new THREE.Mesh(createMeshForShape(shape), material);
			break;
	}

	threeObject.position.copy(wrapVec3(body.GetPosition()));
	threeObject.quaternion.copy(wrapQuat(body.GetRotation()));

	return threeObject;
}

function createMeshFloor(n, cellSize, maxHeight, posX, posY, posZ) {
	// Create regular grid of triangles
	let height = function (x, y) { return Math.sin(x / 2) * Math.cos(y / 3); };
	let triangles = new Jolt.TriangleList;
	triangles.resize(n * n * 2);
	for (let x = 0; x < n; ++x)
		for (let z = 0; z < n; ++z) {
			let center = n * cellSize / 2;

			let x1 = cellSize * x - center;
			let z1 = cellSize * z - center;
			let x2 = x1 + cellSize;
			let z2 = z1 + cellSize;

			{
				let t = triangles.at((x * n + z) * 2);
				let v1 = t.get_mV(0), v2 = t.get_mV(1), v3 = t.get_mV(2);
				v1.x = x1, v1.y = height(x, z), v1.z = z1;
				v2.x = x1, v2.y = height(x, z + 1), v2.z = z2;
				v3.x = x2, v3.y = height(x + 1, z + 1), v3.z = z2;
			}

			{
				let t = triangles.at((x * n + z) * 2 + 1);
				let v1 = t.get_mV(0), v2 = t.get_mV(1), v3 = t.get_mV(2);
				v1.x = x1, v1.y = height(x, z), v1.z = z1;
				v2.x = x2, v2.y = height(x + 1, z + 1), v2.z = z2;
				v3.x = x2, v3.y = height(x + 1, z), v3.z = z1;
			}
		}
	let shape = new Jolt.MeshShapeSettings(triangles, new Jolt.PhysicsMaterialList).Create().Get();

	// Create body
	let creationSettings = new Jolt.BodyCreationSettings(shape, new Jolt.Vec3(posX, posY, posZ), new Jolt.Quat(0, 0, 0, 1), Jolt.EMotionType_Static, LAYER_NON_MOVING);
	let body = bodyInterface.CreateBody(creationSettings);
	addToScene(body, 0xc7c7c7);
}

function createVehicleTrack() {
	const track = [
		[[[38, 64, -14], [38, 64, -16], [38, -64, -16], [38, -64, -14], [64, -64, -16], [64, -64, -14], [64, 64, -16], [64, 64, -14]], [[-16, 64, -14], [-16, 64, -16], [-16, -64, -16], [-16, -64, -14], [10, -64, -16], [10, -64, -14], [10, 64, -16], [10, 64, -14]], [[10, -48, -14], [10, -48, -16], [10, -64, -16], [10, -64, -14], [38, -64, -16], [38, -64, -14], [38, -48, -16], [38, -48, -14]], [[10, 64, -14], [10, 64, -16], [10, 48, -16], [10, 48, -14], [38, 48, -16], [38, 48, -14], [38, 64, -16], [38, 64, -14]]],
		[[[38, 48, -6], [38, 48, -14], [38, -48, -14], [38, -48, -6], [40, -48, -14], [40, -48, -6], [40, 48, -14], [40, 48, -6]], [[62, 62, -6], [62, 62, -14], [62, -64, -14], [62, -64, -6], [64, -64, -14], [64, -64, -6], [64, 62, -14], [64, 62, -6]], [[8, 48, -6], [8, 48, -14], [8, -48, -14], [8, -48, -6], [10, -48, -14], [10, -48, -6], [10, 48, -14], [10, 48, -6]], [[-16, 62, -6], [-16, 62, -14], [-16, -64, -14], [-16, -64, -6], [-14, -64, -14], [-14, -64, -6], [-14, 62, -14], [-14, 62, -6]], [[-14, -62, -6], [-14, -62, -14], [-14, -64, -14], [-14, -64, -6], [62, -64, -14], [62, -64, -6], [62, -62, -14], [62, -62, -6]], [[8, -48, -6], [8, -48, -14], [8, -50, -14], [8, -50, -6], [40, -50, -14], [40, -50, -6], [40, -48, -14], [40, -48, -6]], [[8, 50, -6], [8, 50, -14], [8, 48, -14], [8, 48, -6], [40, 48, -14], [40, 48, -6], [40, 50, -14], [40, 50, -6]], [[-16, 64, -6], [-16, 64, -14], [-16, 62, -14], [-16, 62, -6], [64, 62, -14], [64, 62, -6], [64, 64, -14], [64, 64, -6]]],
		[[[-4, 22, -14], [-4, -14, -14], [-4, -14, -10], [4, -14, -14], [4, -14, -10], [4, 22, -14]], [[-4, -27, -14], [-4, -48, -14], [-4, -48, -11], [4, -48, -14], [4, -48, -11], [4, -27, -14]], [[-4, 50, -14], [-4, 30, -14], [-4, 30, -12], [4, 30, -14], [4, 30, -12], [4, 50, -14]], [[46, 50, -14], [46, 31, -14], [46, 50, -12], [54, 31, -14], [54, 50, -12], [54, 50, -14]], [[46, 16, -14], [46, -19, -14], [46, 16, -10], [54, -19, -14], [54, 16, -10], [54, 16, -14]], [[46, -28, -14], [46, -48, -14], [46, -28, -11], [54, -48, -14], [54, -28, -11], [54, -28, -14]]]
	];

	const mapColors = [0x666666, 0x006600, 0x000066];

	const mapRot = Jolt.Quat.prototype.sRotation(new Jolt.Vec3(0, 1, 0), 0.5 * Math.PI);
	track.forEach((type, tIdx) => {
		type.forEach(block => {
			const hull = new Jolt.ConvexHullShapeSettings;
			block.forEach(v => {
				hull.mPoints.push_back(new Jolt.Vec3(-v[1], v[2], v[0]));
			});
			const shape = hull.Create().Get();
			const creationSettings = new Jolt.BodyCreationSettings(shape, new Jolt.Vec3(0, 10, 0), mapRot, Jolt.EBodyType_Static, LAYER_NON_MOVING);
			const body = bodyInterface.CreateBody(creationSettings);
			body.SetFriction(1.0);
			addToScene(body, mapColors[tIdx]);
		});
	});
}

function addLine(from, to, color) {
	const material = new THREE.LineBasicMaterial({ color: color });
	const points = [];
	points.push(wrapVec3(from));
	points.push(wrapVec3(to));
	const geometry = new THREE.BufferGeometry().setFromPoints(points);
	const line = new THREE.Line(geometry, material);
	scene.add(line);
}

function addMarker(location, size, color) {
	const material = new THREE.LineBasicMaterial({ color: color });
	const points = [];
	const center = wrapVec3(location);
	points.push(center.clone().add(new THREE.Vector3(-size, 0, 0)));
	points.push(center.clone().add(new THREE.Vector3(size, 0, 0)));
	points.push(center.clone().add(new THREE.Vector3(0, -size, 0)));
	points.push(center.clone().add(new THREE.Vector3(0, size, 0)));
	points.push(center.clone().add(new THREE.Vector3(0, 0, -size)));
	points.push(center.clone().add(new THREE.Vector3(0, 0, size)));
	const geometry = new THREE.BufferGeometry().setFromPoints(points);
	const line = new THREE.LineSegments(geometry, material);
	scene.add(line);
}