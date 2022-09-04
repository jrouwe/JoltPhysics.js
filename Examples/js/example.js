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

	container = document.getElementById('container');

	renderer = new THREE.WebGLRenderer();
	renderer.setClearColor(0xbfd1e5);
	renderer.setPixelRatio(window.devicePixelRatio);
	renderer.setSize(window.innerWidth, window.innerHeight);

	camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.2, 2000);
	camera.position.set(0, 10, 25);
	camera.lookAt(new THREE.Vector3(0, 0, 0));

	scene = new THREE.Scene();

	var dirLight = new THREE.DirectionalLight(0xffffff, 1);
	dirLight.position.set(10, 10, 5);
	scene.add(dirLight);

	controls = new THREE.OrbitControls(camera);

	container.innerHTML = "";
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
	// Detects webgl
	if (!Detector.webgl) {
		Detector.addGetWebGLMessage();
		document.getElementById('container').innerHTML = "";
	}

	onExampleUpdate = updateFunction;

	initGraphics();
	initPhysics();
	renderExample();
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
