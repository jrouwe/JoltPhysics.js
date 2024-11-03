#include "Jolt/Renderer/DebugRendererSimple.h"

using BodyManagerDrawSettings = BodyManager::DrawSettings;
using DebugRendererVertex = DebugRenderer::Vertex;
using DebugRendererTriangle = DebugRenderer::Triangle;

using ECullMode = DebugRenderer::ECullMode;
constexpr ECullMode ECullMode_CullBackFace = ECullMode::CullBackFace;
constexpr ECullMode ECullMode_CullFrontFace = ECullMode::CullFrontFace;
constexpr ECullMode ECullMode_Off = ECullMode::Off;

using ECastShadow = DebugRenderer::ECastShadow;
constexpr ECastShadow ECastShadow_On = ECastShadow::On;
constexpr ECastShadow ECastShadow_Off = ECastShadow::Off;

using EDrawMode = DebugRenderer::EDrawMode;
constexpr EDrawMode EDrawMode_Solid = EDrawMode::Solid;
constexpr EDrawMode EDrawMode_Wireframe = EDrawMode::Wireframe;

using EShapeColor = BodyManager::EShapeColor;
constexpr EShapeColor EShapeColor_InstanceColor = EShapeColor::InstanceColor;
constexpr EShapeColor EShapeColor_ShapeTypeColor = EShapeColor::ShapeTypeColor;
constexpr EShapeColor EShapeColor_MotionTypeColor = EShapeColor::MotionTypeColor;
constexpr EShapeColor EShapeColor_SleepColor = EShapeColor::SleepColor;
constexpr EShapeColor EShapeColor_IslandColor = EShapeColor::IslandColor;
constexpr EShapeColor EShapeColor_MaterialColor = EShapeColor::MaterialColor;

constexpr ESoftBodyConstraintColor ESoftBodyConstraintColor_ConstraintType = ESoftBodyConstraintColor::ConstraintType;
constexpr ESoftBodyConstraintColor ESoftBodyConstraintColor_ConstraintGroup = ESoftBodyConstraintColor::ConstraintGroup;
constexpr ESoftBodyConstraintColor ESoftBodyConstraintColor_ConstraintOrder = ESoftBodyConstraintColor::ConstraintOrder;

class DebugRendererVertexTraits
{
public:
	static constexpr uint mPositionOffset = offsetof(DebugRendererVertex, mPosition);
	static constexpr uint mNormalOffset = offsetof(DebugRendererVertex, mNormal);
	static constexpr uint mUVOffset = offsetof(DebugRendererVertex, mUV);
	static constexpr uint mSize = sizeof(DebugRendererVertex);
};

class DebugRendererTriangleTraits
{
public:
	static constexpr uint mVOffset = offsetof(DebugRendererTriangle, mV);
	static constexpr uint mSize = sizeof(DebugRendererTriangle);
};

class DebugRendererEm : public JPH::DebugRenderer
{
public:
	void 			Initialize()
	{
		JPH::DebugRenderer::Initialize();
	}

	virtual void	DrawLine(const RVec3 *inFrom, const RVec3 *inTo, const Color *inColor) = 0;

	virtual void	DrawLine(RVec3Arg inFrom, RVec3Arg inTo, ColorArg inColor)
	{
		DrawLine(&inFrom, &inTo, &inColor);
	}

	virtual void	DrawTriangle(const RVec3 *inV1, const RVec3 *inV2, const RVec3 *inV3, const Color *inColor, ECastShadow inCastShadow = ECastShadow::Off) = 0;

	virtual void	DrawTriangle(RVec3Arg inV1, RVec3Arg inV2, RVec3Arg inV3, ColorArg inColor, ECastShadow inCastShadow = ECastShadow::Off)
	{
		DrawTriangle(&inV1, &inV2, &inV3, &inColor, inCastShadow);
	}

	virtual void	DrawText3D(const RVec3 *inPosition, const void *inString, uint32 inStringLen, const Color *inColor, float inHeight) = 0;

	virtual void	DrawText3D(RVec3Arg inPosition, const string_view &inString, ColorArg inColor, float inHeight)
	{
		DrawText3D(&inPosition, (const void*)inString.data(), inString.size(), &inColor, inHeight);
	}

	virtual uint32	CreateTriangleBatchID(const void *inTriangles, int inTriangleCount) = 0;

	virtual Batch	CreateTriangleBatch(const Triangle *inTriangles, int inTriangleCount)
	{
		uint32 batch = CreateTriangleBatchID((const void*)inTriangles, inTriangleCount);
		return new BatchImpl(batch);
	}

	virtual uint32	CreateTriangleBatchIDWithIndex(const void *inVertices, int inVertexCount, const void *inIndices, int inIndexCount) = 0;

	virtual Batch	CreateTriangleBatch(const Vertex *inVertices, int inVertexCount, const uint32 *inIndices, int inIndexCount)
	{
		uint32 batch = CreateTriangleBatchIDWithIndex((const void*)inVertices, inVertexCount, (const void*)inIndices, inIndexCount);
		return new BatchImpl(batch);
	}

	virtual void	DrawGeometryWithID(const RMat44 *inModelMatrix, const AABox *inWorldSpaceBounds, float inLODScaleSq, Color *inModelColor, const uint32 inGeometryID, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode) = 0;

	virtual void	DrawGeometry(RMat44Arg inModelMatrix, const AABox& inWorldSpaceBounds, float inLODScaleSq, ColorArg inModelColor, const GeometryRef& inGeometry, ECullMode inCullMode, ECastShadow inCastShadow, EDrawMode inDrawMode)
	{
		const LOD *lod = inGeometry->mLODs.data();
		const BatchImpl *batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());

		DrawGeometryWithID(&inModelMatrix, &inWorldSpaceBounds, inLODScaleSq, &inModelColor, batch->mID, inCullMode, inCastShadow, inDrawMode);
	}

	void			DrawBodies(PhysicsSystem *inSystem, BodyManager::DrawSettings *inDrawSettings)
	{
		inSystem->DrawBodies(*inDrawSettings, this);
	}
	void			DrawBodies(PhysicsSystem *inSystem)
	{
		inSystem->DrawBodies(BodyManager::DrawSettings(), this);
	}

	void			DrawConstraints(PhysicsSystem *inSystem)
	{
		inSystem->DrawConstraints(this);
	}

	void			DrawConstraintLimits(PhysicsSystem *inSystem)
	{
		inSystem->DrawConstraintLimits(this);
	}

	void			DrawConstraintReferenceFrame(PhysicsSystem *inSystem)
	{
		inSystem->DrawConstraintReferenceFrame(this);
	}

	void			DrawShape(Shape *inShape, const RMat44 *inModelMatrix, const Vec3 *inScale, const Color *inColor, bool inDrawWireFrame)
	{
		inShape->Draw(this, *inModelMatrix, *inScale, *inColor, false, inDrawWireFrame);
	}

	void			DrawBody(Body *inBody, const Color *inColor, bool inDrawWireFrame)
	{
		RMat44 com = inBody->GetCenterOfMassTransform();
		inBody->GetShape()->Draw(this, com, Vec3::sReplicate(1.0f), *inColor, false, inDrawWireFrame);
	}

	void			DrawConstraint(Constraint *inConstraint)
	{
		inConstraint->DrawConstraint(this);
	}

private:
	/// Implementation specific batch object
	class BatchImpl : public RefTargetVirtual
	{
	public:
		JPH_OVERRIDE_NEW_DELETE

							BatchImpl(uint32 inID) : mID(inID) {  }

		virtual void		AddRef() override { ++mRefCount; }
		virtual void		Release() override { if (--mRefCount == 0) delete this; }

		atomic<uint32>		mRefCount = 0;
		uint32				mID;
	};
};
