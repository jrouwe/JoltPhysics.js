// SPDX-FileCopyrightText: 2022 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include "Jolt/Jolt.h"
#include "Jolt/RegisterTypes.h"
#include "Jolt/Core/Factory.h"
#include "Jolt/Core/JobSystemThreadPool.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Geometry/OrientedBox.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Physics/StateRecorderImpl.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/AABoxCast.h"
#include "Jolt/Physics/Collision/ShapeCast.h"
#include "Jolt/Physics/Collision/CollidePointResult.h"
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
#include "Jolt/Physics/Collision/CollideShape.h"
#include "Jolt/Physics/Constraints/FixedConstraint.h"
#include "Jolt/Physics/Constraints/PointConstraint.h"
#include "Jolt/Physics/Constraints/DistanceConstraint.h"
#include "Jolt/Physics/Constraints/HingeConstraint.h"
#include "Jolt/Physics/Constraints/ConeConstraint.h"
#include "Jolt/Physics/Constraints/SliderConstraint.h"
#include "Jolt/Physics/Constraints/SwingTwistConstraint.h"
#include "Jolt/Physics/Constraints/SixDOFConstraint.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/SoftBody/SoftBodyCreationSettings.h"
#include "Jolt/Physics/SoftBody/SoftBodySharedSettings.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"

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
using CollideShapeResultFace = CollideShapeResult::Face;
using ArraySoftBodySharedSettingsVertex = Array<SoftBodySharedSettingsVertex>;
using ArraySoftBodySharedSettingsFace = Array<SoftBodySharedSettingsFace>;
using ArraySoftBodySharedSettingsEdge = Array<SoftBodySharedSettingsEdge>;
using ArraySoftBodySharedSettingsVolume = Array<SoftBodySharedSettingsVolume>;
using EGroundState = CharacterBase::EGroundState;
using Vector2 = Vector<2>;

// Alias for EBodyType values to avoid clashes
constexpr EBodyType EBodyType_RigidBody = EBodyType::RigidBody;
constexpr EBodyType EBodyType_SoftBody = EBodyType::SoftBody;

// Alias for EMotionType values to avoid clashes
constexpr EMotionType EMotionType_Static = EMotionType::Static;
constexpr EMotionType EMotionType_Kinematic = EMotionType::Kinematic;
constexpr EMotionType EMotionType_Dynamic = EMotionType::Dynamic;

// Alias for EMotionQuality values to avoid clashes
constexpr EMotionQuality EMotionQuality_Discrete = EMotionQuality::Discrete;
constexpr EMotionQuality EMotionQuality_LinearCast = EMotionQuality::LinearCast;

// Alias for EActivation values to avoid clashes
constexpr EActivation EActivation_Activate = EActivation::Activate;
constexpr EActivation EActivation_DontActivate = EActivation::DontActivate;

// Alias for EShapeType values to avoid clashes
constexpr EShapeType EShapeType_Convex = EShapeType::Convex;
constexpr EShapeType EShapeType_Compound = EShapeType::Compound;
constexpr EShapeType EShapeType_Decorated = EShapeType::Decorated;
constexpr EShapeType EShapeType_Mesh = EShapeType::Mesh;
constexpr EShapeType EShapeType_HeightField = EShapeType::HeightField;

// Alias for EShapeSubType values to avoid clashes
constexpr EShapeSubType EShapeSubType_Sphere = EShapeSubType::Sphere;
constexpr EShapeSubType EShapeSubType_Box = EShapeSubType::Box;
constexpr EShapeSubType EShapeSubType_Capsule = EShapeSubType::Capsule;
constexpr EShapeSubType EShapeSubType_TaperedCapsule = EShapeSubType::TaperedCapsule;
constexpr EShapeSubType EShapeSubType_Cylinder = EShapeSubType::Cylinder;
constexpr EShapeSubType EShapeSubType_ConvexHull = EShapeSubType::ConvexHull;
constexpr EShapeSubType EShapeSubType_StaticCompound = EShapeSubType::StaticCompound;
constexpr EShapeSubType EShapeSubType_MutableCompound = EShapeSubType::MutableCompound;
constexpr EShapeSubType EShapeSubType_RotatedTranslated = EShapeSubType::RotatedTranslated;
constexpr EShapeSubType EShapeSubType_Scaled = EShapeSubType::Scaled;
constexpr EShapeSubType EShapeSubType_OffsetCenterOfMass = EShapeSubType::OffsetCenterOfMass;
constexpr EShapeSubType EShapeSubType_Mesh = EShapeSubType::Mesh;
constexpr EShapeSubType EShapeSubType_HeightField = EShapeSubType::HeightField;

// Alias for EConstraintSpace values to avoid clashes
constexpr EConstraintSpace EConstraintSpace_LocalToBodyCOM = EConstraintSpace::LocalToBodyCOM;
constexpr EConstraintSpace EConstraintSpace_WorldSpace = EConstraintSpace::WorldSpace;

// Alias for ESpringMode values to avoid clashes
constexpr ESpringMode ESpringMode_FrequencyAndDamping = ESpringMode::FrequencyAndDamping;
constexpr ESpringMode ESpringMode_StiffnessAndDamping = ESpringMode::StiffnessAndDamping;

// Alias for EOverrideMassProperties values to avoid clashes
constexpr EOverrideMassProperties EOverrideMassProperties_CalculateMassAndInertia = EOverrideMassProperties::CalculateMassAndInertia;
constexpr EOverrideMassProperties EOverrideMassProperties_CalculateInertia = EOverrideMassProperties::CalculateInertia;
constexpr EOverrideMassProperties EOverrideMassProperties_MassAndInertiaProvided = EOverrideMassProperties::MassAndInertiaProvided;

// Alias for EAllowedDOFs values to avoid clashes
constexpr EAllowedDOFs EAllowedDOFs_TranslationX = EAllowedDOFs::TranslationX;
constexpr EAllowedDOFs EAllowedDOFs_TranslationY = EAllowedDOFs::TranslationY;
constexpr EAllowedDOFs EAllowedDOFs_TranslationZ = EAllowedDOFs::TranslationZ;
constexpr EAllowedDOFs EAllowedDOFs_RotationX = EAllowedDOFs::RotationX;
constexpr EAllowedDOFs EAllowedDOFs_RotationY = EAllowedDOFs::RotationY;
constexpr EAllowedDOFs EAllowedDOFs_RotationZ = EAllowedDOFs::RotationZ;
constexpr EAllowedDOFs EAllowedDOFs_Plane2D = EAllowedDOFs::Plane2D;
constexpr EAllowedDOFs EAllowedDOFs_All = EAllowedDOFs::All;

// Alias for EStateRecorderState values to avoid clashes
constexpr EStateRecorderState EStateRecorderState_None = EStateRecorderState::None;
constexpr EStateRecorderState EStateRecorderState_Global = EStateRecorderState::Global;
constexpr EStateRecorderState EStateRecorderState_Bodies = EStateRecorderState::Bodies;
constexpr EStateRecorderState EStateRecorderState_Contacts = EStateRecorderState::Contacts;
constexpr EStateRecorderState EStateRecorderState_Constraints = EStateRecorderState::Constraints;
constexpr EStateRecorderState EStateRecorderState_All = EStateRecorderState::All;

// Alias for EBackFaceMode values to avoid clashes
constexpr EBackFaceMode EBackFaceMode_IgnoreBackFaces = EBackFaceMode::IgnoreBackFaces;
constexpr EBackFaceMode EBackFaceMode_CollideWithBackFaces = EBackFaceMode::CollideWithBackFaces;

// Alias for EGroundState values to avoid clashes
constexpr EGroundState EGroundState_OnGround = EGroundState::OnGround;
constexpr EGroundState EGroundState_OnSteepGround = EGroundState::OnSteepGround;
constexpr EGroundState EGroundState_NotSupported = EGroundState::NotSupported;
constexpr EGroundState EGroundState_InAir = EGroundState::InAir;

// Alias for ValidateResult values to avoid clashes
constexpr ValidateResult ValidateResult_AcceptAllContactsForThisBodyPair = ValidateResult::AcceptAllContactsForThisBodyPair;
constexpr ValidateResult ValidateResult_AcceptContact = ValidateResult::AcceptContact;
constexpr ValidateResult ValidateResult_RejectContact = ValidateResult::RejectContact;
constexpr ValidateResult ValidateResult_RejectAllContactsForThisBodyPair = ValidateResult::RejectAllContactsForThisBodyPair;

// Alias for EActiveEdgeMode values to avoid clashes
constexpr EActiveEdgeMode EActiveEdgeMode_CollideOnlyWithActive = EActiveEdgeMode::CollideOnlyWithActive;
constexpr EActiveEdgeMode EActiveEdgeMode_CollideWithAll = EActiveEdgeMode::CollideWithAll;

// Alias for ECollectFacesMode values to avoid clashes
constexpr ECollectFacesMode ECollectFacesMode_CollectFaces = ECollectFacesMode::CollectFaces;
constexpr ECollectFacesMode ECollectFacesMode_NoFaces = ECollectFacesMode::NoFaces;

// Alias for EConstraintType values to avoid clashes
constexpr EConstraintType EConstraintType_Constraint = EConstraintType::Constraint;
constexpr EConstraintType EConstraintType_TwoBodyConstraint = EConstraintType::TwoBodyConstraint;

// Alias for EConstraintSubType values to avoid clashes
constexpr EConstraintSubType EConstraintSubType_Fixed = EConstraintSubType::Fixed;
constexpr EConstraintSubType EConstraintSubType_Point = EConstraintSubType::Point;
constexpr EConstraintSubType EConstraintSubType_Hinge = EConstraintSubType::Hinge;
constexpr EConstraintSubType EConstraintSubType_Slider = EConstraintSubType::Slider;
constexpr EConstraintSubType EConstraintSubType_Distance = EConstraintSubType::Distance;
constexpr EConstraintSubType EConstraintSubType_Cone = EConstraintSubType::Cone;
constexpr EConstraintSubType EConstraintSubType_SwingTwist = EConstraintSubType::SwingTwist;
constexpr EConstraintSubType EConstraintSubType_SixDOF = EConstraintSubType::SixDOF;
constexpr EConstraintSubType EConstraintSubType_Path = EConstraintSubType::Path;
constexpr EConstraintSubType EConstraintSubType_Vehicle = EConstraintSubType::Vehicle;
constexpr EConstraintSubType EConstraintSubType_RackAndPinion = EConstraintSubType::RackAndPinion;
constexpr EConstraintSubType EConstraintSubType_Gear = EConstraintSubType::Gear;
constexpr EConstraintSubType EConstraintSubType_Pulley = EConstraintSubType::Pulley;

// Alias for SixDOFConstraintSettings::EAxis to avoid clashes
using SixDOFConstraintSettings_EAxis = SixDOFConstraintSettings::EAxis;
constexpr SixDOFConstraintSettings_EAxis SixDOFConstraintSettings_EAxis_TranslationX = SixDOFConstraintSettings_EAxis::TranslationX;
constexpr SixDOFConstraintSettings_EAxis SixDOFConstraintSettings_EAxis_TranslationY = SixDOFConstraintSettings_EAxis::TranslationY;
constexpr SixDOFConstraintSettings_EAxis SixDOFConstraintSettings_EAxis_TranslationZ = SixDOFConstraintSettings_EAxis::TranslationZ;
constexpr SixDOFConstraintSettings_EAxis SixDOFConstraintSettings_EAxis_RotationX = SixDOFConstraintSettings_EAxis::RotationX;
constexpr SixDOFConstraintSettings_EAxis SixDOFConstraintSettings_EAxis_RotationY = SixDOFConstraintSettings_EAxis::RotationY;
constexpr SixDOFConstraintSettings_EAxis SixDOFConstraintSettings_EAxis_RotationZ = SixDOFConstraintSettings_EAxis::RotationZ;

// Alias for EMotorState values to avoid clashes
constexpr EMotorState EMotorState_Off = EMotorState::Off;
constexpr EMotorState EMotorState_Velocity = EMotorState::Velocity;
constexpr EMotorState EMotorState_Position = EMotorState::Position;

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
	virtual bool			ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
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

	virtual uint			GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer	GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < (int)Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *	GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
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
	BroadPhaseLayer			mObjectToBroadPhase[(int)Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool			ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
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

	/// Access to the temp allocator
	TempAllocator *			GetTempAllocator()
	{
		return mTempAllocator;
	}

	/// Access the default object layer pair filter
	ObjectLayerPairFilter *GetObjectLayerPairFilter()
	{
		return &mObjectVsObjectLayerFilter;
	}

	/// Access the default object vs broadphase layer filter
	ObjectVsBroadPhaseLayerFilter *GetObjectVsBroadPhaseLayerFilter()
	{
		return &mObjectVsBroadPhaseLayerFilter;
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

/// A wrapper around ContactListener that is compatible with JavaScript
class ContactListenerEm: public ContactListener
{
public:
	// JavaScript compatible virtual functions
	virtual int				OnContactValidate(const Body &inBody1, const Body &inBody2, const RVec3Arg *inBaseOffset, const CollideShapeResult &inCollisionResult) = 0;

	// Functions that call the JavaScript compatible virtual functions
	virtual ValidateResult	OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
	{ 
		return (ValidateResult)OnContactValidate(inBody1, inBody2, &inBaseOffset, inCollisionResult);
	}
};

/// A wrapper around CharacterContactListener that is compatible with JavaScript
class CharacterContactListenerEm: public CharacterContactListener
{
public:
	// JavaScript compatible virtual functions
	virtual void			OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, Vec3 *inContactPosition, Vec3 *inContactNormal, CharacterContactSettings &ioSettings) = 0;
	virtual void			OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, Vec3 *inContactPosition, Vec3 *inContactNormal, Vec3 *inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3 *inCharacterVelocity, Vec3 &ioNewCharacterVelocity) = 0;

	// Functions that call the JavaScript compatible virtual functions
	virtual void			OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings) override
	{ 
		OnContactAdded(inCharacter, inBodyID2, inSubShapeID2, &inContactPosition, &inContactNormal, ioSettings);
	}

	virtual void			OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3Arg inCharacterVelocity, Vec3 &ioNewCharacterVelocity) override
	{ 
		OnContactSolve(inCharacter, inBodyID2, inSubShapeID2, &inContactPosition, &inContactNormal, &inContactVelocity, inContactMaterial, &inCharacterVelocity, ioNewCharacterVelocity);
	}
};
