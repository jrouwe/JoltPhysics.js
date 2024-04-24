const unwrapV3 = (ptr) => wrapVec3(Jolt.wrapPointer(ptr, Jolt.RVec3));
const textDecoder = new TextDecoder();
class DebugRenderer {
	materialCache = {};
	lineCache = {};
	lineMesh = {};
	triangleCache = {};
	triangleMesh = {};
	meshList = [];
	geometryList = [];
	geometryCache = [];
	textCache = [];
	textList = [];
	renderer;
	css3dRender;
	constructor() {
		const renderer = this.renderer = new Jolt.DebugRendererJS();
		renderer.DrawLine = this.drawLine.bind(this);
		renderer.DrawTriangle = this.drawTriangle.bind(this);
		renderer.DrawText3D = this.drawText3D.bind(this);
		renderer.DrawGeometryWithID = this.drawGeometryWithID.bind(this);
		renderer.CreateTriangleBatchID = this.createTriangleBatchID.bind(this);
		renderer.CreateTriangleBatchIDWithIndex = this.createTriangleBatchIDWithIndex.bind(this);
	}
	initialized = false;
	Initialize() {
		if (!this.initialized) {
			this.renderer.Initialize();
			this.initialized = true;
		}
	}
	// Draws all bodies, assuming DrawSettings has mDrawShape enabled
	DrawBodies(system, inDrawSettings) {
		this.renderer.DrawBodies(system, inDrawSettings);
	}
	// Draws constraint relationships as lines. Some constraints include additional Text Data
	DrawConstraints(system) {
		this.renderer.DrawConstraints(system);
	}
	// Draws text indicating limits on constraints, such as the distance of a distance constraint
	DrawConstraintLimits(system) {
		this.renderer.DrawConstraintLimits(system);
	}

	drawLine(inFrom, inTo, inColor) {
		const colorU32 = Jolt.wrapPointer(inColor, Jolt.Color).mU32 >>> 0;
		const arr = this.lineCache[colorU32] = this.lineCache[colorU32] || [];
		const v0 = unwrapV3(inFrom);
		const v1 = unwrapV3(inTo);
		arr.push(v0, v1);
	}
	drawTriangle(inV1, inV2, inV3, inColor, inCastShadow) {
		const colorU32 = Jolt.wrapPointer(inColor, Jolt.Color).mU32 >>> 0;
		const arr = this.lineCache[colorU32] = this.lineCache[colorU32] || [];
		const v0 = unwrapV3(inV1);
		const v1 = unwrapV3(inV2);
		const v2 = unwrapV3(inV3);
		arr.push(v0, v1);
		arr.push(v1, v2);
		arr.push(v2, v0);
	}
	drawText3D(inPosition, inStringPtr, inStringLen, inColor, inHeight) {
		const color = Jolt.wrapPointer(inColor, Jolt.Color).mU32 >>> 0;
		const position = unwrapV3(inPosition);
		const height = inHeight;
		const text = textDecoder.decode(Jolt.HEAPU8.subarray(inStringPtr, inStringPtr + inStringLen));
		this.textList.push({ color, position, height, text });
	}
	// Assuming a Render Geometry/Batch has been created, the following is a request to render the Geometry at a given model location
	drawGeometryWithID(inModelMatrix, inWorldSpaceBounds, inLODScaleSq, inModelColor, inGeometryID, inCullMode, inCastShadow, inDrawMode) {
		const colorU32 = Jolt.wrapPointer(inModelColor, Jolt.Color).mU32 >>> 0;
		const modelMatrix = Jolt.wrapPointer(inModelMatrix, Jolt.RMat44);
		const v0 = wrapVec3(modelMatrix.GetAxisX());
		const v1 = wrapVec3(modelMatrix.GetAxisY());
		const v2 = wrapVec3(modelMatrix.GetAxisZ());
		const v3 = wrapVec3(modelMatrix.GetTranslation());
		const matrix = new THREE.Matrix4().makeBasis(v0, v1, v2).setPosition(v3);
		this.geometryList.push({ matrix: matrix, geometry: this.geometryCache[inGeometryID], color: colorU32, drawMode: inDrawMode, cullMode: inCullMode });
	}
	// On initializing the Renderer, or adding new rigid Mesh, the following methods will send the vertex data here to construct a Render Geometry
	createTriangleBatchID(inTriangles, inTriangleCount) {
		const batchID = this.geometryCache.length;
		const { mPositionOffset, mNormalOffset, mUVOffset, mSize } = Jolt.DebugRendererVertexTraits.prototype;
		const interleaveBufferF32 = new Float32Array(inTriangleCount * 3 * mSize / 4);

		// Assuming a triangle is tightly packed (always 3 vertex with no leading or trailing space), we can treat the data chunk
		// as a whole as if it was an interleaved vertex buffer, assuming no alignment issues such that element N+1 is more than (size) from element N+0
		// This case is always true as of this coding, but should it not be, the following [else] case will extract just the 3 vertex
		if (Jolt.DebugRendererTriangleTraits.prototype.mVOffset === 0 && Jolt.DebugRendererTriangleTraits.prototype.mSize === mSize * 3) {
			interleaveBufferF32.set(new Float32Array(Jolt.HEAPF32.buffer, inTriangles, interleaveBufferF32.length));
		}
		else {
			const vertexChunk = mSize / 4 * 3;
			for (let i = 0; i < inTriangleCount; i++) {
				let triOffset = inTriangles + i * Jolt.DebugRendererTriangleTraits.prototype.mSize + Jolt.DebugRendererTriangleTraits.prototype.mVOffset;
				interleaveBufferF32.set(new Float32Array(Jolt.HEAPF32.buffer, triOffset, i * vertexChunk));
			}
		}
		// Create a three mesh
		const geometry = new THREE.BufferGeometry();
		const interleavedBuffer = new THREE.InterleavedBuffer(interleaveBufferF32, mSize / 4);
		geometry.setAttribute('position', new THREE.InterleavedBufferAttribute(interleavedBuffer, 3, mPositionOffset / 4));
		geometry.setAttribute('normal', new THREE.InterleavedBufferAttribute(interleavedBuffer, 3, mNormalOffset / 4));
		geometry.setAttribute('uv', new THREE.InterleavedBufferAttribute(interleavedBuffer, 2, mUVOffset / 4));
		this.geometryCache.push(geometry);
		return batchID;
	}
	createTriangleBatchIDWithIndex(inVertices, inVertexCount, inIndices, inIndexCount) {
		const batchID = this.geometryCache.length;
		const { mPositionOffset, mNormalOffset, mUVOffset, mSize } = Jolt.DebugRendererVertexTraits.prototype;
		const interleaveBufferF32 = new Float32Array(inVertexCount * mSize / 4);
		interleaveBufferF32.set(new Float32Array(Jolt.HEAPF32.buffer, inVertices, interleaveBufferF32.length));
		const index = new Uint32Array(inIndexCount);

		// Unlike triangles, by definition this data will be an interleaved data buffer
		index.set(Jolt.HEAPU32.subarray(inIndices / 4, inIndices / 4 + inIndexCount));
		// Create a three mesh
		const geometry = new THREE.BufferGeometry();
		const interleavedBuffer = new THREE.InterleavedBuffer(interleaveBufferF32, mSize / 4);
		geometry.setAttribute('position', new THREE.InterleavedBufferAttribute(interleavedBuffer, 3, mPositionOffset / 4));
		geometry.setAttribute('normal', new THREE.InterleavedBufferAttribute(interleavedBuffer, 3, mNormalOffset / 4));
		geometry.setAttribute('uv', new THREE.InterleavedBufferAttribute(interleavedBuffer, 2, mUVOffset / 4));
		geometry.setIndex(new THREE.BufferAttribute(index, 1));
		this.geometryCache.push(geometry);
		return batchID;
	}
	// Debug Renderer supports applying color, Front and Back face culling, and drawing as solid or wire frame.
	// These all correspond to different Three Materials, so cache them here.
	getMeshMaterial(color, cullMode, drawMode) {
		const key = `${color}|${cullMode}|${drawMode}`;
		if (!this.materialCache[key]) {
			const material = this.materialCache[key] = new THREE.MeshPhongMaterial({ color: color });
			if (drawMode == Jolt.EDrawMode_Wireframe) {
				material.wireframe = true;
			}
			if (cullMode !== undefined) {
				switch (cullMode) {
					case Jolt.ECullMode_Off:
						material.side = THREE.DoubleSide;
						break;
					case Jolt.ECullMode_CullBackFace:
						material.side = THREE.FrontSide;
						break;
					case Jolt.ECullMode_CullFrontFace:
						material.side = THREE.BackSide;
						break;
				}
			}
		}
		return this.materialCache[key];
	}
	// The following call flushes all accumulated Draw calls to new or existing Meshes that have been cached.
	// Line/Triangle calls are combined into single Meshes per material.
	// Text3D calls trigger a lazy initialization of CSS3D Render to render the text as transformed DIVs
	Render() {
		// Clear previous frames meshes, in case this frame no longer has these meshes.
		[Object.values(this.lineMesh), Object.values(this.triangleMesh), this.meshList, this.textCache].forEach(meshes => {
			meshes.forEach(mesh => mesh.visible = false);
		});
		Object.entries(this.lineCache).forEach(([colorU32, points]) => {
			const color = parseInt(colorU32, 10);
			if (this.lineMesh[color]) {
				this.lineMesh[color].geometry = new THREE.BufferGeometry().setFromPoints(points);
				const mesh = this.lineMesh[color];
				mesh.visible = true;
			}
			else {
				const material = new THREE.LineBasicMaterial({ color: color });
				const geometry = new THREE.BufferGeometry().setFromPoints(points);
				const mesh = this.lineMesh[color] = new THREE.LineSegments(geometry, material);
				mesh.layers.set(1);
				scene.add(mesh);
			}
		});
		Object.entries(this.triangleCache).forEach(([colorU32, points]) => {
			const color = parseInt(colorU32, 10);
			if (this.triangleMesh[color]) {
				this.triangleMesh[color].geometry = new THREE.BufferGeometry().setFromPoints(points);
				const mesh = this.triangleMesh[color];
				mesh.visible = true;
			}
			else {
				const material = this.getMeshMaterial(color, undefined, undefined);
				const geometry = new THREE.BufferGeometry().setFromPoints(points);
				const mesh = this.triangleMesh[color] = new THREE.Mesh(geometry, material);
				mesh.layers.set(1);
				scene.add(mesh);
			}
		});
		this.geometryList.forEach(({ geometry, color, matrix, cullMode, drawMode }, i) => {
			const material = this.getMeshMaterial(color, cullMode, drawMode);
			let mesh = this.meshList[i];
			if (!mesh) {
				mesh = this.meshList[i] = new THREE.Mesh(geometry, material);
				mesh.layers.set(1);
				scene.add(mesh);
			}
			else {
				mesh.material = material;
				mesh.geometry = geometry;
			}
			matrix.decompose(mesh.position, mesh.quaternion, mesh.scale);
			mesh.visible = true;
		});
		this.textList.forEach(({ position, text, color, height }, i) => {
			let mesh = this.textCache[i];
			if (!this.css3dRender) {
				// Lazy construct a CSS3D Renderer.
				this.css3dRender = new THREE.CSS3DRenderer();
				const renderSize = new THREE.Vector2();
				renderer.getSize(renderSize);
				this.css3dRender.setSize(renderSize.x, renderSize.y);
				const element = this.css3dRender.domElement;
				element.style.position = 'absolute';
				element.style.left = element.style.right = element.style.top = element.style.bottom = '0';
				document.getElementById('container')?.append(element);
				window.addEventListener('resize', () => { renderer.getSize(renderSize); this.css3dRender.setSize(renderSize.x, renderSize.y); }, false);
			}
			if (!mesh) {
				mesh = this.textCache[i] = new THREE.CSS3DObject(document.createElement('div'));
				mesh.element.style.display = 'block';
				mesh.element.style.fontSize = '1px';
				mesh.layers.set(1);
				scene.add(mesh);
			}
			else {
				mesh.element.innerText = text;
				mesh.element.style.color = '#' + ('000000' + color.toString(16)).substr(-6);
			}
			mesh.position.copy(position);
			mesh.visible = true;
		});
		// Render the CSS 3D here (updates the DIV locations and css transforms)
		this.css3dRender && this.css3dRender.render(scene, camera);
		// Clear the accumulators of [Draw] requests
		this.geometryList = [];
		this.textList = [];
		this.lineCache = {};
		this.triangleCache = {};
	}
}

// The following is a non-exhaustive list of options that may be provided to the DebugRender's DrawBodies call
const BodyDrawSettingsMap = [
	{ key: 'mDrawShape', label: 'Shape' },
	{ key: 'mDrawShapeWireframe', label: 'Shape Wireframe' },
	{
		key: 'mDrawShapeColor', label: 'Shape Color', options: [
			{ key: "EShapeColor_InstanceColor", label: 'Instance Color' },
			{ key: "EShapeColor_ShapeTypeColor", label: 'Shape Type Color' },
			{ key: "EShapeColor_MotionTypeColor", label: 'Motion Color' },
			{ key: "EShapeColor_SleepColor", label: 'Sleep Color' },
			{ key: "EShapeColor_IslandColor", label: 'Island Color' },
			{ key: "EShapeColor_MaterialColor", label: 'Material Color' }
		]
	},
	{ key: 'mDrawBoundingBox', label: 'Bounding Box' },
	{ key: 'mDrawCenterOfMassTransform', label: 'Center of Mass Transform' },
	{ key: 'mDrawWorldTransform', label: 'World Transform' },
	{ key: 'mDrawVelocity', label: 'Velocity' },
	{ key: 'mDrawMassAndInertia', label: 'Mass And Inertia' },
	{ key: 'mDrawSleepStats', label: 'Sleep Stats' },
	{ header: 'Soft Body' },
	{ key: 'mDrawSoftBodyVertices', label: 'Vertices' },
	{ key: 'mDrawSoftBodyVertexVelocities', label: 'Vertex Velocities' },
	{ key: 'mDrawSoftBodyEdgeConstraints', label: 'Edge Constraints' },
	{ key: 'mDrawSoftBodyBendConstraints', label: 'Bend Constraints' },
	{ key: 'mDrawSoftBodyVolumeConstraints', label: 'Volume Constraints' },
	{ key: 'mDrawSoftBodySkinConstraints', label: 'Skin Constraints' },
	{ key: 'mDrawSoftBodyLRAConstraints', label: 'LRA Constraints' }
];


class RenderWidget {
	renderer;
	bodyDrawSettings;
	domElement;
	drawConstraints = true;
	drawBodies = true;
	constructor(jolt) {
		window.Jolt = jolt;
		this.renderer = new DebugRenderer();
		this.bodyDrawSettings = new Jolt.BodyManagerDrawSettings();
		this.domElement = document.createElement('div');
		this.domElement.className = "debug-renderer collapsed";
		this.domElement.innerHTML = `
	<style>
	.debug-renderer { z-index: 2; position: absolute; right: 0; top: 120px; max-width: 300px; background: rgba(0,0,0,.8);color: white; padding: 5px; border: 2px solid white; border-radius: 8px; }
	.debug-renderer label { display: block }
	.debug-renderer > b { display: block; padding: 5px; text-align: center; }
	.debug-renderer.collapsed label, .debug-renderer.collapsed h4, .debug-renderer.collapsed select { display: none}
	.debug-render option { font-size: 16px }
	</style>
	<b>Debug Menu <button>SHOW</button></b>`
		let collapsed = true;
		const button = this.domElement.querySelector('button');
		button.onclick = () => { this.domElement.classList.toggle('collapsed'); collapsed = !collapsed; button.innerText = collapsed ? 'SHOW' : 'HIDE'; };
	}
	init() {
		// The scene and all existing lights should be visible in Layers 0x1 and 0x2
		scene.traverse((x) => { if (x.isLight) x.layers.mask = 3 });
		scene.layers.mask = 3;
		const renderMask = document.createElement('select');
		renderMask.innerHTML = `<option value='1' selected>ORIGINAL</option><option value='2'>DEBUG</option><option value='3'>BOTH</option>`;
		renderMask.onchange = () => { camera.layers.mask = parseInt(renderMask.value, 10); }
		this.domElement.append(renderMask);
		this.addCheckBox('Draw Bodies', this.drawBodies, (checked) => this.drawBodies = checked);
		this.addCheckBox('Draw Constraints', this.drawConstraints, (checked) => this.drawConstraints = checked);
		BodyDrawSettingsMap.forEach(item => {
			if (item.header) {
				const header = document.createElement('h4');
				header.innerText = item.header;
				this.domElement.append(header);
			}
			else {
				if (item.options) {
					const label = document.createElement('label');
					label.innerText = item.label + '\n';
					this.domElement.append(label);
					const options = document.createElement('select');
					item.options.forEach(option => {
						const o = document.createElement('option');
						o.innerText = option.label;
						o.value = option.key;
						options.append(o);
					});
					options.onchange = () => {
						this.bodyDrawSettings[item.key] = Jolt[options.value];
					};
					label.append(options);
				}
				else {
					this.addCheckBox(item.label, this.bodyDrawSettings[item.key], (checked) => this.bodyDrawSettings[item.key] = checked);
				}
			}
		});
	}
	addCheckBox(labelText, initialValue, onChange) {
		const label = document.createElement('label');
		label.innerText = labelText;
		this.domElement.append(label);
		const check = document.createElement('input');
		check.type = 'checkbox';
		check.checked = initialValue;
		check.onclick = () => { onChange(check.checked); };
		label.append(check);
	}
	render() {
		this.renderer.Initialize();
		if (this.drawBodies)
			this.renderer.DrawBodies(physicsSystem, this.bodyDrawSettings);
		if (this.drawConstraints) {
			this.renderer.DrawConstraints(physicsSystem);
			this.renderer.DrawConstraintLimits(physicsSystem);
		}
		this.renderer.Render();
	}
}
