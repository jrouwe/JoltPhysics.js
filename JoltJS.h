// SPDX-FileCopyrightText: 2022 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/StateRecorderImpl.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/TaperedCapsuleShape.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "Jolt/Physics/Collision/GroupFilterTable.h"
#include "Jolt/Physics/Constraints/FixedConstraint.h"
#include "Jolt/Physics/Constraints/PointConstraint.h"
#include "Jolt/Physics/Constraints/DistanceConstraint.h"
#include "Jolt/Physics/Constraints/HingeConstraint.h"
#include "Jolt/Physics/Constraints/ConeConstraint.h"
#include "Jolt/Physics/Constraints/SliderConstraint.h"
#include "Jolt/Physics/Constraints/SwingTwistConstraint.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/SoftBody/SoftBodyCreationSettings.h"
#include "Jolt/Physics/SoftBody/SoftBodySharedSettings.h"

#include <iostream>

using namespace JPH;
using namespace std;

// Types that need to be exposed to JavaScript
using JPHString = String;
using ArrayVec3 = Array<Vec3>;
using SoftBodySharedSettingsVertex = SoftBodySharedSettings::Vertex;
using SoftBodySharedSettingsFace = SoftBodySharedSettings::Face;
using SoftBodySharedSettingsEdge = SoftBodySharedSettings::Edge;
using SoftBodySharedSettingsVolume = SoftBodySharedSettings::Volume;
using ArraySoftBodySharedSettingsVertex = Array<SoftBodySharedSettingsVertex>;
using ArraySoftBodySharedSettingsFace = Array<SoftBodySharedSettingsFace>;
using ArraySoftBodySharedSettingsEdge = Array<SoftBodySharedSettingsEdge>;
using ArraySoftBodySharedSettingsVolume = Array<SoftBodySharedSettingsVolume>;

// Callback for traces
static void TraceImpl(const char *inFMT, ...)
{ 
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);

	// Print to the TTY
	cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{ 
	// Print to the TTY
	cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr? inMessage : "") << endl;

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with.
enum class Layers
{
	NON_MOVING = 0,
	MOVING = 1,
	NUM_LAYERS = 2
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool					ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case (int)Layers::NON_MOVING:
			return inObject2 == (int)Layers::MOVING; // Non moving only collides with moving
		case (int)Layers::MOVING:
			return true; // Moving collides with everything
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase.
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
									BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[(int)Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[(int)Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint					GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer			GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < (int)Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *			GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:	return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:		return "MOVING";
		default:													JPH_ASSERT(false); return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer					mObjectToBroadPhase[(int)Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool					ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
		case (int)Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case (int)Layers::MOVING:
			return true;	
		default:
			JPH_ASSERT(false);
			return false;
		}
	}
};

/// Settings to pass to constructor
class JoltSettings
{
public:
	uint					mMaxBodies = 10240;
	uint					mMaxBodyPairs = 65536;
	uint					mMaxContactConstraints = 10240;
	uint					mTempAllocatorSize = 10 * 1024 * 1024;
};

/// Main API for JavaScript
class JoltInterface
{
public:
	/// Constructor
							JoltInterface(const JoltSettings &inSettings)
	{
		// Install callbacks
		Trace = TraceImpl;
		JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

		// Create a factory
		Factory::sInstance = new Factory();

		// Register all Jolt physics types
		RegisterTypes();

		// Init temp allocator
		mTempAllocator = new TempAllocatorImpl(inSettings.mTempAllocatorSize);
		
		// Init the physics system
		constexpr uint cNumBodyMutexes = 0;
		mPhysicsSystem.Init(inSettings.mMaxBodies, cNumBodyMutexes, inSettings.mMaxBodyPairs, inSettings.mMaxContactConstraints, mBPLayerInterface, mObjectVsBroadPhaseLayerFilter, mObjectVsObjectLayerFilter);
	}

	/// Destructor
							~JoltInterface()
	{
		// Destroy subsystems
		delete mTempAllocator;
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
	}

	/// Step the world
	void					Step(float inDeltaTime, int inCollisionSteps)
	{
		mPhysicsSystem.Update(inDeltaTime, inCollisionSteps, mTempAllocator, &mJobSystem);
	}

	/// Access to the physics system
	PhysicsSystem *			GetPhysicsSystem()
	{
		return &mPhysicsSystem;
	}

private:
	TempAllocatorImpl *		mTempAllocator;
	JobSystemThreadPool		mJobSystem { cMaxPhysicsJobs, cMaxPhysicsBarriers, (int)thread::hardware_concurrency() - 1 };
	BPLayerInterfaceImpl	mBPLayerInterface;
	ObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadPhaseLayerFilter;
	ObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;
	PhysicsSystem			mPhysicsSystem;
};

/// Helper class to extract triangles from the shape
class ShapeGetTriangles
{
public:
							ShapeGetTriangles(const Shape *inShape, const AABox &inBox, Vec3Arg inPositionCOM, QuatArg inRotation, Vec3Arg inScale)
	{
		const size_t cBlockSize = 8096;

		// First collect all leaf shapes
		AllHitCollisionCollector<TransformedShapeCollector> collector;
		inShape->CollectTransformedShapes(inBox, inPositionCOM, inRotation, inScale, SubShapeIDCreator(), collector, { });

		size_t cur_pos = 0;

		// Iterate the leaf shapes
		for (const TransformedShape &ts : collector.mHits)
		{
			// Start iterating triangles
			Shape::GetTrianglesContext context;
			ts.GetTrianglesStart(context, inBox, RVec3::sZero());

			for (;;)
			{
				// Ensure we have space to get more triangles
				size_t tri_left = mMaterials.size() - cur_pos;
				if (tri_left < Shape::cGetTrianglesMinTrianglesRequested)
				{
					mVertices.resize(mVertices.size() + 3 * cBlockSize);
					mMaterials.resize(mMaterials.size() + cBlockSize);
					tri_left = mMaterials.size() - cur_pos;
				}

				// Fetch next batch
				int count = ts.GetTrianglesNext(context, tri_left, mVertices.data() + 3 * cur_pos, mMaterials.data() + cur_pos);
				if (count == 0)
				{
					// We're done
					mVertices.resize(3 * cur_pos);
					mMaterials.resize(cur_pos);
					break;
				}

				cur_pos += count;
			}
		}

		// Free excess memory
		mVertices.shrink_to_fit();
		mMaterials.shrink_to_fit();
	}

	int						GetNumTriangles() const
	{
		return (int)mMaterials.size();
	}

	int						GetVerticesSize() const
	{
		return (int)mVertices.size() * sizeof(Float3);
	}

	const Float3 *			GetVerticesData() const
	{
		return mVertices.data();
	}
		
	const PhysicsMaterial *	GetMaterial(int inTriangle) const
	{
		return mMaterials[inTriangle];
	}
	
private:
	Array<Float3>			mVertices;
	Array<const PhysicsMaterial *>	mMaterials;
};
