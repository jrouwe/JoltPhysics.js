class SoftBodyCreator {
	static CreateCloth(inGridSizeX = 30, inGridSizeZ = 30, inGridSpacing = 0.75, inVertexGetInvMass = (_x, _y) => 1, inVertexPerturbation = (_x, _z) => ({ x: 0, y: 0, z: 0 }), inBendType = Jolt.SoftBodySharedSettings_EBendType_None, inVertexAttributes) {
		const cOffsetX = -0.5 * inGridSpacing * (inGridSizeX - 1);
		const cOffsetZ = -0.5 * inGridSpacing * (inGridSizeZ - 1);

		// Create settings
		const sharedSettings = new Jolt.SoftBodySharedSettings;
		const v = new Jolt.SoftBodySharedSettingsVertex;
		for (let z = 0; z < inGridSizeZ; ++z)
			for (let x = 0; x < inGridSizeX; ++x) {
				const perturb = inVertexPerturbation(x, z);
				v.mPosition.x = inGridSpacing * x + cOffsetX + perturb.x;
				v.mPosition.y = 0 + perturb.y;
				v.mPosition.z = inGridSpacing * z + cOffsetZ + perturb.z;
				v.mInvMass = inVertexGetInvMass(x, z);
				sharedSettings.mVertices.push_back(v);
			}
		Jolt.destroy(v);

		// Function to get the vertex index of a point on the cloth
		function vertex_index(inX, inY) {
			return inX + inY * inGridSizeX;
		}
		sharedSettings.CalculateEdgeLengths();

		// Create faces
		const f = new Jolt.SoftBodySharedSettingsFace(0, 0, 0, 0);
		for (let z = 0; z < inGridSizeZ - 1; ++z)
			for (let x = 0; x < inGridSizeX - 1; ++x) {
				f.set_mVertex(0, vertex_index(x, z));
				f.set_mVertex(1, vertex_index(x, z + 1));
				f.set_mVertex(2, vertex_index(x + 1, z + 1));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(x + 1, z + 1));
				f.set_mVertex(2, vertex_index(x + 1, z));
				sharedSettings.AddFace(f);
			}
		Jolt.destroy(f);

		if (inVertexAttributes) {
			sharedSettings.CreateConstraints(inVertexAttributes, 1, inBendType);
		} else {
			const inVertexAttributes = new Jolt.SoftBodySharedSettingsVertexAttributes();
			inVertexAttributes.mCompliance = 1.0e-5;
			inVertexAttributes.mShearCompliance = 1.0e-5;
			inVertexAttributes.mBendCompliance = 1.0e-5;
			sharedSettings.CreateConstraints(inVertexAttributes, 1, inBendType);
			Jolt.destroy(inVertexAttributes);
		}

		// Optimize the settings
		sharedSettings.Optimize();
		return sharedSettings;
	}

	static CreateClothWithFixatedCorners(inGridSizeX = 30, inGridSizeZ = 30, inGridSpacing = 0.75) {
		function inv_mass(inX, inZ) {
			return (inX == 0 && inZ == 0)
				|| (inX == inGridSizeX - 1 && inZ == 0)
				|| (inX == 0 && inZ == inGridSizeZ - 1)
				|| (inX == inGridSizeX - 1 && inZ == inGridSizeZ - 1) ? 0.0 : 1.0;
		};

		return this.CreateCloth(inGridSizeX, inGridSizeZ, inGridSpacing, inv_mass);
	}

	static CreateCube(inGridSize = 5, inGridSpacing = 0.5, edgeCompliance = 0, volumeCompliance = 0) {
		const cOffset = -0.5 * inGridSpacing * (inGridSize - 1);

		// Create settings
		const sharedSettings = new Jolt.SoftBodySharedSettings;
		const v = new Jolt.SoftBodySharedSettingsVertex;
		for (let z = 0; z < inGridSize; ++z)
			for (let y = 0; y < inGridSize; ++y)
				for (let x = 0; x < inGridSize; ++x) {
					v.mPosition.x = inGridSpacing * x + cOffset;
					v.mPosition.y = inGridSpacing * y + cOffset;
					v.mPosition.z = inGridSpacing * z + cOffset;
					sharedSettings.mVertices.push_back(v);
				}
		Jolt.destroy(v);

		// Function to get the vertex index of a point on the cloth
		const vertex_index = (inX, inY, inZ) => {
			return inX + inY * inGridSize + inZ * inGridSize * inGridSize;
		};
		const sEdge = new Jolt.SoftBodySharedSettingsEdge(0, 0, 0);
		sEdge.mCompliance = edgeCompliance;

		// Create edges
		for (let z = 0; z < inGridSize; ++z)
			for (let y = 0; y < inGridSize; ++y)
				for (let x = 0; x < inGridSize; ++x) {
					const v0 = vertex_index(x, y, z);
					sEdge.set_mVertex(0, v0);
					if (x < inGridSize - 1) {
						const v1 = vertex_index(x + 1, y, z);
						sEdge.set_mVertex(1, v1);
						sharedSettings.mEdgeConstraints.push_back(sEdge);
					}
					if (y < inGridSize - 1) {
						const v1 = vertex_index(x, y + 1, z);
						sEdge.set_mVertex(1, v1);
						sharedSettings.mEdgeConstraints.push_back(sEdge);
					}
					if (z < inGridSize - 1) {
						const v1 = vertex_index(x, y, z + 1);
						sEdge.set_mVertex(1, v1);
						sharedSettings.mEdgeConstraints.push_back(sEdge);
					}
				}
		Jolt.destroy(sEdge);
		sharedSettings.CalculateEdgeLengths();

		const tetra_indices = [
			[[0, 0, 0], [0, 1, 1], [0, 0, 1], [1, 1, 1]],
			[[0, 0, 0], [0, 1, 0], [0, 1, 1], [1, 1, 1]],
			[[0, 0, 0], [0, 0, 1], [1, 0, 1], [1, 1, 1]],
			[[0, 0, 0], [1, 0, 1], [1, 0, 0], [1, 1, 1]],
			[[0, 0, 0], [1, 1, 0], [0, 1, 0], [1, 1, 1]],
			[[0, 0, 0], [1, 0, 0], [1, 1, 0], [1, 1, 1]]
		];

		// Create volume constraints
		const sVol = new Jolt.SoftBodySharedSettingsVolume(0, 0, 0, 0, 0);
		sVol.mCompliance = volumeCompliance;
		for (let z = 0; z < inGridSize - 1; ++z)
			for (let y = 0; y < inGridSize - 1; ++y)
				for (let x = 0; x < inGridSize - 1; ++x)
					for (let t = 0; t < 6; ++t) {
						for (let i = 0; i < 4; ++i)
							sVol.set_mVertex(i, vertex_index(x + tetra_indices[t][i][0], y + tetra_indices[t][i][1], z + tetra_indices[t][i][2]));
						sharedSettings.mVolumeConstraints.push_back(sVol);
					}
		Jolt.destroy(sVol);
		sharedSettings.CalculateVolumeConstraintVolumes();

		// Create faces
		const f = new Jolt.SoftBodySharedSettingsFace(0, 0, 0, 0);
		for (let y = 0; y < inGridSize - 1; ++y)
			for (let x = 0; x < inGridSize - 1; ++x) {
				// Face 1
				f.set_mVertex(0, vertex_index(x, y, 0));
				f.set_mVertex(1, vertex_index(x, y + 1, 0));
				f.set_mVertex(2, vertex_index(x + 1, y + 1, 0));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(x + 1, y + 1, 0));
				f.set_mVertex(2, vertex_index(x + 1, y, 0));
				sharedSettings.AddFace(f);
				// Face 2
				f.set_mVertex(0, vertex_index(x, y, inGridSize - 1));
				f.set_mVertex(1, vertex_index(x + 1, y + 1, inGridSize - 1));
				f.set_mVertex(2, vertex_index(x, y + 1, inGridSize - 1));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(x + 1, y, inGridSize - 1));
				f.set_mVertex(2, vertex_index(x + 1, y + 1, inGridSize - 1));
				sharedSettings.AddFace(f);
				// Face 3
				f.set_mVertex(0, vertex_index(x, 0, y));
				f.set_mVertex(1, vertex_index(x + 1, 0, y + 1));
				f.set_mVertex(2, vertex_index(x, 0, y + 1));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(x + 1, 0, y));
				f.set_mVertex(2, vertex_index(x + 1, 0, y + 1));
				sharedSettings.AddFace(f);
				// Face 4
				f.set_mVertex(0, vertex_index(x, inGridSize - 1, y));
				f.set_mVertex(1, vertex_index(x, inGridSize - 1, y + 1));
				f.set_mVertex(2, vertex_index(x + 1, inGridSize - 1, y + 1));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(x + 1, inGridSize - 1, y + 1));
				f.set_mVertex(2, vertex_index(x + 1, inGridSize - 1, y));
				sharedSettings.AddFace(f);
				// Face 5
				f.set_mVertex(0, vertex_index(0, x, y));
				f.set_mVertex(1, vertex_index(0, x, y + 1));
				f.set_mVertex(2, vertex_index(0, x + 1, y + 1));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(0, x + 1, y + 1));
				f.set_mVertex(2, vertex_index(0, x + 1, y));
				sharedSettings.AddFace(f);
				// Face 6
				f.set_mVertex(0, vertex_index(inGridSize - 1, x, y));
				f.set_mVertex(1, vertex_index(inGridSize - 1, x + 1, y + 1));
				f.set_mVertex(2, vertex_index(inGridSize - 1, x, y + 1));
				sharedSettings.AddFace(f);
				f.set_mVertex(1, vertex_index(inGridSize - 1, x + 1, y));
				f.set_mVertex(2, vertex_index(inGridSize - 1, x + 1, y + 1));
				sharedSettings.AddFace(f);
			}
		Jolt.destroy(f);

		// Optimize the settings
		sharedSettings.Optimize();
		return sharedSettings;
	}

	static CreateSphere(inRadius = 1, inNumTheta = 10, inNumPhi = 20, inBendType = Jolt.SoftBodySharedSettings_EBendType_None, inVertexAttributes) {
		const sharedSettings = new Jolt.SoftBodySharedSettings;
		const v3 = new window.THREE.Vector3();

		// Create settings
		// NOTE: This is not how you should create a soft body sphere, we explicitly use polar coordinates to make the vertices unevenly distributed.
		// Doing it this way tests the pressure algorithm as it receives non-uniform triangles. Better is to use uniform triangles,
		const v = new Jolt.SoftBodySharedSettingsVertex;
		function sUnitSpherical(phi, theta) {
			v3.setFromSphericalCoords(inRadius, phi, theta);
			v.mPosition.x = v3.x;
			v.mPosition.y = v3.y;
			v.mPosition.z = v3.z;
			sharedSettings.mVertices.push_back(v);
		}
		sUnitSpherical(0, 0);
		sUnitSpherical(Math.PI, 0);
		for (let theta = 1; theta < inNumTheta - 1; ++theta)
			for (let phi = 0; phi < inNumPhi; ++phi) {
				sUnitSpherical(Math.PI * theta / (inNumTheta - 1), 2.0 * Math.PI * phi / inNumPhi);
			}
		Jolt.destroy(v);

		function vertex_index(inTheta, inPhi) {
			if (inTheta == 0)
				return 0;
			else if (inTheta == inNumTheta - 1)
				return 1;
			else
				return 2 + (inTheta - 1) * inNumPhi + inPhi % inNumPhi;
		}
		const f = new Jolt.SoftBodySharedSettingsFace(0, 0, 0, 0);
		for (let phi = 0; phi < inNumPhi; ++phi) {
			for (let theta = 0; theta < inNumTheta - 2; ++theta) {
				f.set_mVertex(0, vertex_index(theta, phi));
				f.set_mVertex(1, vertex_index(theta + 1, phi));
				f.set_mVertex(2, vertex_index(theta + 1, phi + 1));
				sharedSettings.AddFace(f);
				if (theta > 0) {
					f.set_mVertex(1, vertex_index(theta + 1, phi + 1));
					f.set_mVertex(2, vertex_index(theta, phi + 1));
					sharedSettings.AddFace(f);
				}
			}
			f.set_mVertex(0, vertex_index(inNumTheta - 2, phi + 1));
			f.set_mVertex(1, vertex_index(inNumTheta - 2, phi));
			f.set_mVertex(2, vertex_index(inNumTheta - 1, 0));
			sharedSettings.AddFace(f);
		}
		Jolt.destroy(f);

		if (inVertexAttributes) {
			sharedSettings.CreateConstraints(inVertexAttributes, 1, inBendType);
		} else {
			const inVertexAttributes = new Jolt.SoftBodySharedSettingsVertexAttributes();
			inVertexAttributes.mCompliance = 1.0e-4;
			inVertexAttributes.mShearCompliance = 1.0e-4;
			inVertexAttributes.mBendCompliance = 1.0e-3;
			sharedSettings.CreateConstraints(inVertexAttributes, 1, inBendType);
			Jolt.destroy(inVertexAttributes);
		}

		// Optimize the settings
		sharedSettings.Optimize();
		return sharedSettings;
	}
}
