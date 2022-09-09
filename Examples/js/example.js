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

// The update function
var onExampleUpdate;

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

	// Initialize Jolt
	jolt = new Jolt.JoltInterface();
	physicsSystem = jolt.GetPhysicsSystem();
	bodyInterface = physicsSystem.GetBodyInterface();
}

function updatePhysics(deltaTime) {

	// Don't go below 30 Hz to prevent spiral of death
	deltaTime = Math.min(deltaTime, 1.0 / 30.0);

	// When running below 55 Hz, do 2 steps instead of 1
	var numSteps = deltaTime > 1.0 / 55.0 ? 2 : 1;

	// Step the physics world
	jolt.Step(deltaTime, numSteps, 1);
}

function initExample(updateFunction) {
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

	var deltaTime = clock.getDelta();

	onExampleUpdate(time, deltaTime);

	time += deltaTime;

	updatePhysics(deltaTime);

	controls.update(deltaTime);

	renderer.render(scene, camera);

	stats.update();
}

function createFloor() {
	// Create floor
	scene.add(new THREE.Mesh(new THREE.BoxGeometry(100, 1, 100, 1, 1, 1), new THREE.MeshPhongMaterial({ color: 0xC7C7C7 })));

	// Create corresponding physics object
	var shape = new Jolt.BoxShape(new Jolt.Vec3(50, 0.5, 50), 0.001, null);
	var creation_settings = new Jolt.BodyCreationSettings(shape, new Jolt.Vec3(0, 0, 0), new Jolt.Quat(0, 0, 0, 1), Jolt.Static, Jolt.NON_MOVING);
	bodyInterface.CreateAndAddBody(creation_settings, Jolt.DontActivate);
}

function getThreeMeshForShape(shape, material) {
	// Get triangle data
	let triContext = new Jolt.ShapeGetTriangles(shape, Jolt.AABox.prototype.sBiggest(), shape.GetCenterOfMass(), Jolt.Quat.prototype.sIdentity(), new Jolt.Vec3(1, 1, 1));

	// Get a view on the triangle data (does not make a copy)
	let vertices = new Float32Array(Jolt.HEAPF32.buffer, triContext.GetVerticesData(), triContext.GetVerticesSize());

	// Create a three object
	let geometry = new THREE.BufferGeometry();
	geometry.setAttribute('position', new THREE.BufferAttribute(vertices, 3));
	geometry.computeVertexNormals();
	return new THREE.Mesh(geometry, material);
}

function createMeshFloor(n, cell_size, max_height, posX, posY, posZ) {
	// Create regular grid of triangles
	let height = function (x, y) { return Math.sin(x / 2) * Math.cos(y / 3); };
	let triangles = new Jolt.TriangleList;
	triangles.resize(n * n * 2);
	for (let x = 0; x < n; ++x)
		for (let z = 0; z < n; ++z) {
			let center = n * cell_size / 2;

			let x1 = cell_size * x - center;
			let z1 = cell_size * z - center;
			let x2 = x1 + cell_size;
			let z2 = z1 + cell_size;

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
	let shape = new Jolt.MeshShapeSettings(triangles, null).Create().Get();

	// Create body
	let creation_settings = new Jolt.BodyCreationSettings(shape, new Jolt.Vec3(posX, posY, posZ), new Jolt.Quat(0, 0, 0, 1), Jolt.Static, Jolt.NON_MOVING);
	bodyInterface.CreateAndAddBody(creation_settings, Jolt.DontActivate);

	// Create corresponding three mesh
	let threeObject = getThreeMeshForShape(shape, new THREE.MeshPhongMaterial({ color: 0xC7C7C7 }));
	threeObject.position.set(posX, posY, posZ);
	scene.add(threeObject);
}