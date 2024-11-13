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
#include "Jolt/Physics/Collision/Shape/TaperedCylinderShape.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "Jolt/Physics/Collision/Shape/StaticCompoundShape.h"
#include "Jolt/Physics/Collision/Shape/MutableCompoundShape.h"
#include "Jolt/Physics/Collision/Shape/ScaledShape.h"
#include "Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/Collision/Shape/HeightFieldShape.h"
#include "Jolt/Physics/Collision/Shape/PlaneShape.h"
#include "Jolt/Physics/Collision/Shape/EmptyShape.h"
#include "Jolt/Physics/Collision/CollisionCollectorImpl.h"
#include "Jolt/Physics/Collision/GroupFilterTable.h"
#include "Jolt/Physics/Collision/CollideShape.h"
#include "Jolt/Physics/Constraints/FixedConstraint.h"
#include "Jolt/Physics/Constraints/PointConstraint.h"
#include "Jolt/Physics/Constraints/DistanceConstraint.h"
#include "Jolt/Physics/Constraints/HingeConstraint.h"
#include "Jolt/Physics/Constraints/ConeConstraint.h"
#include "Jolt/Physics/Constraints/PathConstraint.h"
#include "Jolt/Physics/Constraints/PathConstraintPathHermite.h"
#include "Jolt/Physics/Constraints/PulleyConstraint.h"
#include "Jolt/Physics/Constraints/SliderConstraint.h"
#include "Jolt/Physics/Constraints/SwingTwistConstraint.h"
#include "Jolt/Physics/Constraints/SixDOFConstraint.h"
#include "Jolt/Physics/Constraints/GearConstraint.h"
#include "Jolt/Physics/Constraints/RackAndPinionConstraint.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Ragdoll/Ragdoll.h"
#include "Jolt/Physics/SoftBody/SoftBodyCreationSettings.h"
#include "Jolt/Physics/SoftBody/SoftBodySharedSettings.h"
#include "Jolt/Physics/SoftBody/SoftBodyShape.h"
#include "Jolt/Physics/SoftBody/SoftBodyMotionProperties.h"
#include "Jolt/Physics/SoftBody/SoftBodyContactListener.h"
#include "Jolt/Physics/SoftBody/SoftBodyManifold.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Vehicle/VehicleConstraint.h"
#include "Jolt/Physics/Vehicle/MotorcycleController.h"
#include "Jolt/Physics/Vehicle/TrackedVehicleController.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h"
#include "Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h"
#include "Jolt/Physics/Collision/ObjectLayerPairFilterTable.h"
#include "Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceMask.h"
#include "Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterMask.h"
#include "Jolt/Physics/Collision/ObjectLayerPairFilterMask.h"
#include "Jolt/Physics/Body/BodyActivationListener.h"
#include "Jolt/Skeleton/SkeletalAnimation.h"
#include "Jolt/Skeleton/SkeletonPose.h"
#include "Jolt/Skeleton/Skeleton.h"

#include <iostream>
#include <malloc.h>
#include <unistd.h>
#include <emscripten/em_asm.h>

using namespace JPH;
using namespace std;

#ifdef JPH_DEBUG_RENDERER
	#include "JoltJS-DebugRenderer.h"
#endif

// Ensure that we use 32-bit object layers
static_assert(sizeof(ObjectLayer) == 4);

// Types that need to be exposed to JavaScript
using JPHString = String;
using ArrayVec3 = Array<Vec3>;
using ArrayFloat = Array<float>;
using ArrayUint = Array<uint>;
using ArrayUint8 = Array<uint8>;
using Vec3MemRef = Vec3;
using QuatMemRef = Quat;
using ArrayQuat = Array<Quat>;
using Mat44MemRef = Mat44;
using ArrayMat44 = Array<Mat44>;
using BodyIDMemRef = BodyID;
using ArrayBodyID = Array<BodyID>;
using BodyPtrMemRef = Body *;
using ArrayBodyPtr = Array<Body *>;
using FloatMemRef = float;
using UintMemRef = uint;
using Uint8MemRef = uint8;
using SoftBodySharedSettingsVertex = SoftBodySharedSettings::Vertex;
using SoftBodySharedSettingsFace = SoftBodySharedSettings::Face;
using SoftBodySharedSettingsEdge = SoftBodySharedSettings::Edge;
using SoftBodySharedSettingsDihedralBend = SoftBodySharedSettings::DihedralBend;
using SoftBodySharedSettingsVolume = SoftBodySharedSettings::Volume;
using SoftBodySharedSettingsInvBind = SoftBodySharedSettings::InvBind;
using SoftBodySharedSettingsSkinWeight = SoftBodySharedSettings::SkinWeight;
using SoftBodySharedSettingsSkinned = SoftBodySharedSettings::Skinned;
using SoftBodySharedSettingsLRA = SoftBodySharedSettings::LRA;
using SoftBodySharedSettingsVertexAttributes = SoftBodySharedSettings::VertexAttributes;
using CollideShapeResultFace = CollideShapeResult::Face;
using ArraySoftBodySharedSettingsVertex = Array<SoftBodySharedSettingsVertex>;
using ArraySoftBodySharedSettingsFace = Array<SoftBodySharedSettingsFace>;
using ArraySoftBodySharedSettingsEdge = Array<SoftBodySharedSettingsEdge>;
using ArraySoftBodySharedSettingsDihedralBend = Array<SoftBodySharedSettingsDihedralBend>;
using ArraySoftBodySharedSettingsVolume = Array<SoftBodySharedSettingsVolume>;
using ArraySoftBodySharedSettingsInvBind = Array<SoftBodySharedSettingsInvBind>;
using ArraySoftBodySharedSettingsSkinWeight = Array<SoftBodySharedSettingsSkinWeight>;
using ArraySoftBodySharedSettingsSkinned = Array<SoftBodySharedSettingsSkinned>;
using ArraySoftBodySharedSettingsLRA = Array<SoftBodySharedSettingsLRA>;
using ArraySoftBodySharedSettingsVertexAttributes = Array<SoftBodySharedSettingsVertexAttributes>;
using ArraySoftBodyVertex = Array<SoftBodyVertex>;
using EGroundState = CharacterBase::EGroundState;
using Vector2 = Vector<2>;
using ArrayRayCastResult = Array<RayCastResult>;
using CastRayAllHitCollisionCollector = AllHitCollisionCollector<CastRayCollector>;
using CastRayClosestHitCollisionCollector = ClosestHitCollisionCollector<CastRayCollector>;
using CastRayAnyHitCollisionCollector = AnyHitCollisionCollector<CastRayCollector>;
using ArrayCollidePointResult = Array<CollidePointResult>;
using CollidePointAllHitCollisionCollector = AllHitCollisionCollector<CollidePointCollector>;
using CollidePointClosestHitCollisionCollector = ClosestHitCollisionCollector<CollidePointCollector>;
using CollidePointAnyHitCollisionCollector = AnyHitCollisionCollector<CollidePointCollector>;
using ArrayCollideShapeResult = Array<CollideShapeResult>;
using CollideShapeAllHitCollisionCollector = AllHitCollisionCollector<CollideShapeCollector>;
using CollideShapeClosestHitCollisionCollector = ClosestHitCollisionCollector<CollideShapeCollector>;
using CollideShapeAnyHitCollisionCollector = AnyHitCollisionCollector<CollideShapeCollector>;
using ArrayShapeCastResult = Array<ShapeCastResult>;
using CastShapeAllHitCollisionCollector = AllHitCollisionCollector<CastShapeCollector>;
using CastShapeClosestHitCollisionCollector = ClosestHitCollisionCollector<CastShapeCollector>;
using CastShapeAnyHitCollisionCollector = AnyHitCollisionCollector<CastShapeCollector>;
using ArrayWheelSettings = Array<Ref<WheelSettings>>;
using ArrayVehicleAntiRollBar = Array<VehicleAntiRollBar>;
using ArrayVehicleDifferentialSettings = Array<VehicleDifferentialSettings>;
using SkeletalAnimationJointState = SkeletalAnimation::JointState;
using SkeletalAnimationKeyframe = SkeletalAnimation::Keyframe;
using SkeletalAnimationAnimatedJoint = SkeletalAnimation::AnimatedJoint;
using ArraySkeletonKeyframe = Array<SkeletalAnimationKeyframe>;
using ArraySkeletonAnimatedJoint = Array<SkeletalAnimationAnimatedJoint>;
using RagdollPart = RagdollSettings::Part;
using RagdollAdditionalConstraint = RagdollSettings::AdditionalConstraint;
using ArrayRagdollPart = Array<RagdollPart>;
using ArrayRagdollAdditionalConstraint = Array<RagdollAdditionalConstraint>;
using CompoundShapeSubShape = CompoundShape::SubShape;
using BodyInterface_AddState = void;

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
constexpr EShapeType EShapeType_Plane = EShapeType::Plane;
constexpr EShapeType EShapeType_Empty = EShapeType::Empty;

// Alias for EShapeSubType values to avoid clashes
constexpr EShapeSubType EShapeSubType_Sphere = EShapeSubType::Sphere;
constexpr EShapeSubType EShapeSubType_Box = EShapeSubType::Box;
constexpr EShapeSubType EShapeSubType_Capsule = EShapeSubType::Capsule;
constexpr EShapeSubType EShapeSubType_TaperedCapsule = EShapeSubType::TaperedCapsule;
constexpr EShapeSubType EShapeSubType_Cylinder = EShapeSubType::Cylinder;
constexpr EShapeSubType EShapeSubType_TaperedCylinder = EShapeSubType::TaperedCylinder;
constexpr EShapeSubType EShapeSubType_ConvexHull = EShapeSubType::ConvexHull;
constexpr EShapeSubType EShapeSubType_StaticCompound = EShapeSubType::StaticCompound;
constexpr EShapeSubType EShapeSubType_MutableCompound = EShapeSubType::MutableCompound;
constexpr EShapeSubType EShapeSubType_RotatedTranslated = EShapeSubType::RotatedTranslated;
constexpr EShapeSubType EShapeSubType_Scaled = EShapeSubType::Scaled;
constexpr EShapeSubType EShapeSubType_OffsetCenterOfMass = EShapeSubType::OffsetCenterOfMass;
constexpr EShapeSubType EShapeSubType_Mesh = EShapeSubType::Mesh;
constexpr EShapeSubType EShapeSubType_HeightField = EShapeSubType::HeightField;
constexpr EShapeSubType EShapeSubType_Plane = EShapeSubType::Plane;
constexpr EShapeSubType EShapeSubType_Empty = EShapeSubType::Empty;

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

// Alias for SoftBodyValidateResult values to avoid clashes
constexpr SoftBodyValidateResult SoftBodyValidateResult_AcceptContact = SoftBodyValidateResult::AcceptContact;
constexpr SoftBodyValidateResult SoftBodyValidateResult_RejectContact = SoftBodyValidateResult::RejectContact;

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

/// Alias for EPathRotationConstraintType to avoid clash
constexpr EPathRotationConstraintType EPathRotationConstraintType_Free = EPathRotationConstraintType::Free; 
constexpr EPathRotationConstraintType EPathRotationConstraintType_ConstrainAroundTangent = EPathRotationConstraintType::ConstrainAroundTangent;
constexpr EPathRotationConstraintType EPathRotationConstraintType_ConstrainAroundNormal = EPathRotationConstraintType::ConstrainAroundNormal;
constexpr EPathRotationConstraintType EPathRotationConstraintType_ConstrainAroundBinormal = EPathRotationConstraintType::ConstrainAroundBinormal;
constexpr EPathRotationConstraintType EPathRotationConstraintType_ConstrainToPath = EPathRotationConstraintType::ConstrainToPath;
constexpr EPathRotationConstraintType EPathRotationConstraintType_FullyConstrained = EPathRotationConstraintType::FullyConstrained;

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

// Alias for ETransmissionMode values to avoid clashes
constexpr ETransmissionMode ETransmissionMode_Auto = ETransmissionMode::Auto;
constexpr ETransmissionMode ETransmissionMode_Manual = ETransmissionMode::Manual;

// Defining ETireFrictionDirection since we cannot pass references to float
enum ETireFrictionDirection
{
	ETireFrictionDirection_Longitudinal,
	ETireFrictionDirection_Lateral
};

// Alias for ESwingType values to avoid clashes
constexpr ESwingType ESwingType_Cone = ESwingType::Cone;
constexpr ESwingType ESwingType_Pyramid = ESwingType::Pyramid;

// Alias for EBendType values to avoid clashes
using SoftBodySharedSettings_EBendType = SoftBodySharedSettings::EBendType;
constexpr SoftBodySharedSettings_EBendType SoftBodySharedSettings_EBendType_None = SoftBodySharedSettings::EBendType::None;
constexpr SoftBodySharedSettings_EBendType SoftBodySharedSettings_EBendType_Distance = SoftBodySharedSettings::EBendType::Distance;
constexpr SoftBodySharedSettings_EBendType SoftBodySharedSettings_EBendType_Dihedral = SoftBodySharedSettings::EBendType::Dihedral;

// Alias for ELRAType values to avoid clashes
using SoftBodySharedSettings_ELRAType = SoftBodySharedSettings::ELRAType;
constexpr SoftBodySharedSettings_ELRAType SoftBodySharedSettings_ELRAType_None = SoftBodySharedSettings::ELRAType::None;
constexpr SoftBodySharedSettings_ELRAType SoftBodySharedSettings_ELRAType_EuclideanDistance = SoftBodySharedSettings::ELRAType::EuclideanDistance;
constexpr SoftBodySharedSettings_ELRAType SoftBodySharedSettings_ELRAType_GeodesicDistance = SoftBodySharedSettings::ELRAType::GeodesicDistance;

// Helper class to store information about the memory layout of SoftBodyVertex
class SoftBodyVertexTraits
{
public:
	static constexpr uint mPreviousPositionOffset = offsetof(SoftBodyVertex, mPreviousPosition);
	static constexpr uint mPositionOffset = offsetof(SoftBodyVertex, mPosition);
	static constexpr uint mVelocityOffset = offsetof(SoftBodyVertex, mVelocity);
};

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

/// Settings to pass to constructor
class JoltSettings
{
public:
	uint					mMaxBodies = 10240;
	uint					mMaxBodyPairs = 65536;
	uint					mMaxContactConstraints = 10240;
	uint					mTempAllocatorSize = 10 * 1024 * 1024;
	uint					mMaxWorkerThreads = 16;
	BroadPhaseLayerInterface *mBroadPhaseLayerInterface = nullptr;
	ObjectVsBroadPhaseLayerFilter *mObjectVsBroadPhaseLayerFilter = nullptr;
	ObjectLayerPairFilter *	mObjectLayerPairFilter = nullptr;
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

		// Limit to 16 threads since we limit the webworker thread pool size to this as well
		int num_workers = min<int>(thread::hardware_concurrency() - 1, min<int>(inSettings.mMaxWorkerThreads, 16));
		mJobSystem = new JobSystemThreadPool(cMaxPhysicsJobs, cMaxPhysicsBarriers, num_workers); 

		// Check required objects
		if (inSettings.mBroadPhaseLayerInterface == nullptr || inSettings.mObjectVsBroadPhaseLayerFilter == nullptr || inSettings.mObjectLayerPairFilter == nullptr)
			Trace("Error: BroadPhaseLayerInterface, ObjectVsBroadPhaseLayerFilter and ObjectLayerPairFilter must be provided");

		// Store interfaces
		mBroadPhaseLayerInterface = inSettings.mBroadPhaseLayerInterface;
		mObjectVsBroadPhaseLayerFilter = inSettings.mObjectVsBroadPhaseLayerFilter;
		mObjectLayerPairFilter = inSettings.mObjectLayerPairFilter;

		// Init the physics system
		constexpr uint cNumBodyMutexes = 0;
		mPhysicsSystem = new PhysicsSystem();
		mPhysicsSystem->Init(inSettings.mMaxBodies, cNumBodyMutexes, inSettings.mMaxBodyPairs, inSettings.mMaxContactConstraints, *inSettings.mBroadPhaseLayerInterface, *inSettings.mObjectVsBroadPhaseLayerFilter, *inSettings.mObjectLayerPairFilter);
	}

	/// Destructor
							~JoltInterface()
	{
		// Destroy subsystems
		delete mPhysicsSystem;
		delete mBroadPhaseLayerInterface;
		delete mObjectVsBroadPhaseLayerFilter;
		delete mObjectLayerPairFilter;
		delete mJobSystem;
		delete mTempAllocator;
		delete Factory::sInstance;
		Factory::sInstance = nullptr;
		UnregisterTypes();
	}

	/// Step the world
	void					Step(float inDeltaTime, int inCollisionSteps)
	{
		mPhysicsSystem->Update(inDeltaTime, inCollisionSteps, mTempAllocator, mJobSystem);
	}

	/// Access to the physics system
	PhysicsSystem *			GetPhysicsSystem()
	{
		return mPhysicsSystem;
	}

	/// Access to the temp allocator
	TempAllocator *			GetTempAllocator()
	{
		return mTempAllocator;
	}

	/// Access the default object layer pair filter
	ObjectLayerPairFilter *GetObjectLayerPairFilter()
	{
		return mObjectLayerPairFilter;
	}

	/// Access the default object vs broadphase layer filter
	ObjectVsBroadPhaseLayerFilter *GetObjectVsBroadPhaseLayerFilter()
	{
		return mObjectVsBroadPhaseLayerFilter;
	}

	/// Get the total reserved memory in bytes
	/// See: https://github.com/emscripten-core/emscripten/blob/7459cab167138419168b5ac5eacf74702d5a3dae/test/core/test_mallinfo.c#L16-L18
	static size_t			sGetTotalMemory()
	{
		return (size_t)EM_ASM_PTR(return HEAP8.length);
	}

	/// Get the amount of free memory in bytes
	/// See: https://github.com/emscripten-core/emscripten/blob/7459cab167138419168b5ac5eacf74702d5a3dae/test/core/test_mallinfo.c#L20-L25
	static size_t			sGetFreeMemory()
	{
		struct mallinfo i = mallinfo();
		uintptr_t total_memory = sGetTotalMemory();
		uintptr_t dynamic_top = (uintptr_t)sbrk(0);
		return total_memory - dynamic_top + i.fordblks;
	}

private:
	TempAllocatorImpl *		mTempAllocator = nullptr;
	JobSystemThreadPool *	mJobSystem = nullptr;
	BroadPhaseLayerInterface *mBroadPhaseLayerInterface = nullptr;
	ObjectVsBroadPhaseLayerFilter *mObjectVsBroadPhaseLayerFilter = nullptr;
	ObjectLayerPairFilter *	mObjectLayerPairFilter = nullptr;
	PhysicsSystem *			mPhysicsSystem = nullptr;
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
	virtual int				OnContactValidate(const Body &inBody1, const Body &inBody2, const RVec3 *inBaseOffset, const CollideShapeResult &inCollisionResult) = 0;

	// Functions that call the JavaScript compatible virtual functions
	virtual ValidateResult	OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
	{ 
		return (ValidateResult)OnContactValidate(inBody1, inBody2, &inBaseOffset, inCollisionResult);
	}
};

/// A wrapper around SoftBodyContactListener that is compatible with JavaScript
class SoftBodyContactListenerEm: public SoftBodyContactListener
{
public:
	// JavaScript compatible virtual functions
	virtual int				OnSoftBodyContactValidate(const Body &inSoftBody, const Body &inOtherBody, SoftBodyContactSettings *ioSettings) = 0;

	// Functions that call the JavaScript compatible virtual functions
	virtual SoftBodyValidateResult	OnSoftBodyContactValidate(const Body &inSoftBody, const Body &inOtherBody, SoftBodyContactSettings &ioSettings)
	{ 
		return (SoftBodyValidateResult)OnSoftBodyContactValidate(inSoftBody, inOtherBody, &ioSettings);
	}
};

/// A wrapper around CharacterContactListener that is compatible with JavaScript
class CharacterContactListenerEm: public CharacterContactListener
{
public:
	// JavaScript compatible virtual functions
	virtual void			OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, const RVec3 *inContactPosition, const Vec3 *inContactNormal, CharacterContactSettings &ioSettings) = 0;
	virtual void			OnCharacterContactAdded(const CharacterVirtual *inCharacter, const CharacterVirtual *inOtherCharacter, const SubShapeID &inSubShapeID2, const RVec3 *inContactPosition, const Vec3 *inContactNormal, CharacterContactSettings &ioSettings) = 0;
	virtual void			OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, const RVec3 *inContactPosition, const Vec3 *inContactNormal, const Vec3 *inContactVelocity, const PhysicsMaterial *inContactMaterial, const Vec3 *inCharacterVelocity, Vec3 &ioNewCharacterVelocity) = 0;
	virtual void			OnCharacterContactSolve(const CharacterVirtual *inCharacter, const CharacterVirtual *inOtherCharacter, const SubShapeID &inSubShapeID2, const RVec3 *inContactPosition, const Vec3 *inContactNormal, const Vec3 *inContactVelocity, const PhysicsMaterial *inContactMaterial, const Vec3 *inCharacterVelocity, Vec3 &ioNewCharacterVelocity) = 0;

	// Functions that call the JavaScript compatible virtual functions
	virtual void			OnContactAdded(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings) override
	{ 
		OnContactAdded(inCharacter, inBodyID2, inSubShapeID2, &inContactPosition, &inContactNormal, ioSettings);
	}

	virtual void			OnCharacterContactAdded(const CharacterVirtual *inCharacter, const CharacterVirtual *inOtherCharacter, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, CharacterContactSettings &ioSettings) override
	{ 
		OnCharacterContactAdded(inCharacter, inOtherCharacter, inSubShapeID2, &inContactPosition, &inContactNormal, ioSettings);
	}

	virtual void			OnContactSolve(const CharacterVirtual *inCharacter, const BodyID &inBodyID2, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3Arg inCharacterVelocity, Vec3 &ioNewCharacterVelocity) override
	{ 
		OnContactSolve(inCharacter, inBodyID2, inSubShapeID2, &inContactPosition, &inContactNormal, &inContactVelocity, inContactMaterial, &inCharacterVelocity, ioNewCharacterVelocity);
	}

	virtual void			OnCharacterContactSolve(const CharacterVirtual *inCharacter, const CharacterVirtual *inOtherCharacter, const SubShapeID &inSubShapeID2, RVec3Arg inContactPosition, Vec3Arg inContactNormal, Vec3Arg inContactVelocity, const PhysicsMaterial *inContactMaterial, Vec3Arg inCharacterVelocity, Vec3 &ioNewCharacterVelocity) override
	{ 
		OnCharacterContactSolve(inCharacter, inOtherCharacter, inSubShapeID2, &inContactPosition, &inContactNormal, &inContactVelocity, inContactMaterial, &inCharacterVelocity, ioNewCharacterVelocity);
	}
};

/// A wrapper around the physics step listener that is compatible with JavaScript (JS doesn't like multiple inheritance)
class VehicleConstraintStepListener : public PhysicsStepListener
{
public:
							VehicleConstraintStepListener(VehicleConstraint *inVehicleConstraint)
	{
		mInstance = inVehicleConstraint;
	}

	virtual void			OnStep(const PhysicsStepListenerContext &inContext) override
	{
		PhysicsStepListener *instance = mInstance;
		instance->OnStep(inContext);
	}

private:
	VehicleConstraint *		mInstance;
};

/// Wrapper class around ObjectVsBroadPhaseLayerFilter to make it compatible with JavaScript (JS cannot pass parameter by value)
class ObjectVsBroadPhaseLayerFilterEm : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool			ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer *inLayer2) const = 0;

	virtual bool			ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const
	{
		return ShouldCollide(inLayer1, &inLayer2);
	}
};

/// Wrapper class around BroadPhaseLayerInterface to make it compatible with JavaScript (JS cannot return parameter by value)
class BroadPhaseLayerInterfaceEm : public BroadPhaseLayerInterface
{
public:
	virtual unsigned short	GetBPLayer(ObjectLayer inLayer) const = 0;

	virtual BroadPhaseLayer	GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		return BroadPhaseLayer(GetBPLayer(inLayer));
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	/// Get the user readable name of a broadphase layer (debugging purposes)
	virtual const char *	GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		return "Undefined";
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED
};

/// A wrapper around the vehicle constraint callbacks that is compatible with JavaScript
class VehicleConstraintCallbacksEm
{
public:
	virtual					~VehicleConstraintCallbacksEm() = default;

	void					SetVehicleConstraint(VehicleConstraint &inConstraint)
	{
		inConstraint.SetCombineFriction([this](uint inWheelIndex, float &ioLongitudinalFriction, float &ioLateralFriction, const Body &inBody2, const SubShapeID &inSubShapeID2) {
			ioLongitudinalFriction = GetCombinedFriction(inWheelIndex, ETireFrictionDirection_Longitudinal, ioLongitudinalFriction, inBody2, inSubShapeID2);
			ioLateralFriction = GetCombinedFriction(inWheelIndex, ETireFrictionDirection_Lateral, ioLateralFriction, inBody2, inSubShapeID2);
		});
		inConstraint.SetPreStepCallback([this](VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext) {
			OnPreStepCallback(inVehicle, inContext);
		});
		inConstraint.SetPostCollideCallback([this](VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext) {
			OnPostCollideCallback(inVehicle, inContext);
		});
		inConstraint.SetPostStepCallback([this](VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext) {
			OnPostStepCallback(inVehicle, inContext);
		});
	}

	virtual float			GetCombinedFriction(unsigned int inWheelIndex, ETireFrictionDirection inTireFrictionDirection, float inTireFriction, const Body &inBody2, const SubShapeID &inSubShapeID2) = 0;
	virtual void			OnPreStepCallback(VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext) = 0;
	virtual void			OnPostCollideCallback(VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext) = 0;
	virtual void			OnPostStepCallback(VehicleConstraint &inVehicle, const PhysicsStepListenerContext &inContext) = 0;
};

/// The tire max impulse callback returns multiple parameters, so we need to store them in a class
class TireMaxImpulseCallbackResult
{
public:
	float					mLongitudinalImpulse;
	float					mLateralImpulse;
};

/// A wrapper around the wheeled vehicle controller callbacks that is compatible with JavaScript
class WheeledVehicleControllerCallbacksEm
{
public:
	virtual					~WheeledVehicleControllerCallbacksEm() = default;

	void					SetWheeledVehicleController(WheeledVehicleController &inController)
	{
		inController.SetTireMaxImpulseCallback([this](uint inWheelIndex, float &outLongitudinalImpulse, float &outLateralImpulse, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float inLongitudinalSlip, float inLateralSlip, float inDeltaTime) {
			// Pre-fill the structure with default calculated values
			TireMaxImpulseCallbackResult result;
			result.mLongitudinalImpulse = inLongitudinalFriction * inSuspensionImpulse;
			result.mLateralImpulse = inLateralFriction * inSuspensionImpulse;

			OnTireMaxImpulseCallback(inWheelIndex, &result, inSuspensionImpulse, inLongitudinalFriction, inLateralFriction, inLongitudinalSlip, inLateralSlip, inDeltaTime);

			// Read the results
			outLongitudinalImpulse = result.mLongitudinalImpulse;
			outLateralImpulse = result.mLateralImpulse;
		});
	}

	virtual void			OnTireMaxImpulseCallback(uint inWheelIndex, TireMaxImpulseCallbackResult *outResult, float inSuspensionImpulse, float inLongitudinalFriction, float inLateralFriction, float inLongitudinalSlip, float inLateralSlip, float inDeltaTime) = 0;
};

class PathConstraintPathEm: public PathConstraintPath
{
public:
	virtual float 			GetClosestPoint(Vec3Arg inPosition, float inFractionHint) const
	{
		return GetClosestPoint(&inPosition, inFractionHint);
	}
	
	virtual void 			GetPointOnPath(float inFraction, Vec3 &outPathPosition, Vec3 &outPathTangent, Vec3 &outPathNormal, Vec3 &outPathBinormal) const
	{
		GetPointOnPath(inFraction, &outPathPosition, &outPathTangent, &outPathNormal, &outPathBinormal);
	}

	virtual float 			GetClosestPoint(const Vec3 *inPosition, float inFractionHint) const = 0;
	virtual void 			GetPointOnPath(float inFraction, Vec3 *outPathPosition, Vec3 *outPathTangent, Vec3 *outPathNormal, Vec3 *outPathBinormal) const = 0;
};

class StateRecorderEm : public StateRecorder
{
public:
	// Unfortunately WebIDL doesn't support size_t so we need to map it to unsigned int
	virtual void		WriteBytes(const void *inData, unsigned int inNumBytes) = 0;
	virtual void		ReadBytes(void *outData, unsigned int inNumBytes) = 0;

private:
	virtual void		WriteBytes(const void *inData, size_t inNumBytes) override
	{
		WriteBytes(inData, (unsigned int)inNumBytes);
	}

	virtual void		ReadBytes(void *outData, size_t inNumBytes) override
	{
		ReadBytes(outData, (unsigned int)inNumBytes);
	}
};

class HeightFieldShapeConstantValues 
{
public:
	/// Value used to create gaps in the height field
	static constexpr float	cNoCollisionValue = HeightFieldShapeConstants::cNoCollisionValue;
};
