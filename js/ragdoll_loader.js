
const RagdollLoader = {
		load: async function (Jolt, filename, EMotionType, sConstraintType) {
				const S = (string) => new Jolt.JPHString(string, string.length);

				const text = await fetch(filename).then(res => res.text());
				const objects = ObjectStreamIn.sReadObject(text);

				const { mParts, mSkeleton } = objects.result;
				const skeleton = new Jolt.Skeleton();
				const bones = mSkeleton.mJoints.map(bone => bone.mName);
				mSkeleton.mJoints.forEach(joint => {
						skeleton.AddJoint(S(joint.mName), bones.indexOf(joint.mParentName));
				});

				const vec3 = new Jolt.Vec3();
				const quat = new Jolt.Quat();
				const vec4 = new Jolt.Vec4();

				const setValues = (target, source) => {
						Object.entries(source).forEach(([name, value]) => {
								const valueType = typeof value;
								if (target[name] !== undefined && value) {
										if (valueType == 'number' || valueType == 'boolean') {
												target[name] = value;
										} else if (value._type) {
												switch (value._type) {
														case 'vec3':
														case 'quat':
																target[name].Set(...value.value);
																break;
														case 'mat44':
																const m = target[name];
																const v = value.value;
																for (let i = 0; i < 4; i++) {
																		vec4.Set(v.slice(i * 4, (i + 1) * 4));
																		m.SetColumn4(i, vec4);
																}
																break;
												}
										} else if (valueType == 'object' && Jolt.getPointer(target[name]) > 0) {
												setValues(target[name], value)
										}
								}
						})
				}

				function getShape(tofShape) {
						switch (tofShape._class) {
								case 'StaticCompoundShapeSettings': {
										const settings = new Jolt.StaticCompoundShapeSettings();
										setValues(settings, tofShape);
										tofShape.mSubShapes.forEach(subShape => {
												const subShapeSettings = getShape(subShape.mShape);
												vec3.Set(...subShape.mPosition.value);
												quat.Set(...subShape.mRotation.value);
												settings.AddShape(vec3, quat, subShapeSettings, subShape.mUserData);
										});
										return settings;
								}
										break;
								case 'CapsuleShapeSettings': {
										const { mHalfHeightOfCylinder, mRadius } = tofShape;
										const settings = new Jolt.CapsuleShapeSettings(mHalfHeightOfCylinder, mRadius);
										setValues(settings, tofShape);
										return settings;
								}
										break;
								case 'TaperedCapsuleShapeSettings': {
										const { mHalfHeightOfTaperedCylinder, mTopRadius, mBottomRadius } = tofShape;
										const settings = new Jolt.TaperedCapsuleShapeSettings(mHalfHeightOfTaperedCylinder, mTopRadius, mBottomRadius);
										setValues(settings, tofShape);
										return settings;
								}
										break;
								case 'BoxShapeSettings': {
										const { mHalfExtent, mConvexRadius } = tofShape;
										vec3.Set(...mHalfExtent.value);
										const settings = new Jolt.BoxShapeSettings(vec3, mConvexRadius);
										setValues(settings, tofShape);
										return settings;
								}
										break;
						}
				}
				function overrideConstraint(original, type) {
						let settings;
						switch (type) {
								case "Fixed":
										settings = new Jolt.FixedConstraintSettings();
										settings.mPoint1 = settings.mPoint2 = original.mPosition1;
										break;
								case "Point":
										settings = new Jolt.PointConstraintSettings();
										settings.mPoint1 = settings.mPoint2 = original.mPosition1;
										break;
								case "Hinge":
										settings = new Jolt.HingeConstraintSettings();
										settings.mPoint1 = original.mPosition1;
										settings.mHingeAxis1 = original.mPlaneAxis1;
										settings.mNormalAxis1 = original.mTwistAxis1;
										settings.mPoint2 = original.mPosition2;
										settings.mHingeAxis2 = original.mPlaneAxis2;
										settings.mNormalAxis2 = original.mTwistAxis2;
										settings.mLimitsMin = -original.mNormalHalfConeAngle;
										settings.mLimitsMax = original.mNormalHalfConeAngle;
										settings.mMaxFrictionTorque = original.mMaxFrictionTorque;
										break;
								case "Slider":
										settings = new Jolt.SliderConstraintSettings();
										settings.mPoint1 = settings.mPoint2 = original.mPosition1;
										settings.mSliderAxis1 = settings.mSliderAxis2 = original.mTwistAxis1;
										settings.mNormalAxis1 = settings.mNormalAxis2 = original.mTwistAxis1.GetNormalizedPerpendicular();
										settings.mLimitsMin = -1.0;
										settings.mLimitsMax = 1.0;
										settings.mMaxFrictionForce = original.mMaxFrictionTorque;
										break;
								case "Cone":
										settings = new Jolt.ConeConstraintSettings();
										settings.mPoint1 = original.mPosition1;
										settings.mTwistAxis1 = original.mTwistAxis1;
										settings.mPoint2 = original.mPosition2;
										settings.mTwistAxis2 = original.mTwistAxis2;
										settings.mHalfConeAngle = original.mNormalHalfConeAngle;
										break;
								case "Ragdoll":
										settings = original;
										break;
						}
						if (settings && original != settings) {
								Jolt.destroy(original);
								return settings;
						} else {
								return original;
						}
				}

				const ragdoll = new Jolt.RagdollSettings();
				ragdoll.mSkeleton = skeleton;
				ragdoll.mParts.resize(skeleton.GetJointCount());
				for (let p = 0; p < skeleton.GetJointCount(); ++p) {
						const part = ragdoll.mParts.at(p);
						const mPart = mParts[p];
						setValues(part, mPart);
						const shapeSettings = getShape(mPart.mShape);
						const shapeResult = shapeSettings.Create();
						if (shapeResult.HasError()) {
								throw new Error('Shape Error', shapeResult.GetError().c_str());
						}
						Jolt.destroy(shapeSettings);

						part.mObjectLayer = LAYER_MOVING;
						part.mMotionType = EMotionType;
						part.mCollisionGroup.SetGroupID(mPart.mCollisionGroup.mGroupID);
						part.mCollisionGroup.SetSubGroupID(mPart.mCollisionGroup.mSubGroupID);

						part.SetShape(shapeResult.Get());
						const constraintType = mPart.mToParent && mPart.mToParent._class;
						if (constraintType && Jolt[constraintType]) {
								const constraintSettings = new Jolt[constraintType];
								setValues(constraintSettings, mPart.mToParent);
								const constraint = overrideConstraint(constraintSettings, sConstraintType);
								['mMotorSettings', 'mSwingMotorSettings', 'mTwistMotorSettings'].forEach(field => {
										const motorSetting = constraint[field]
										if (motorSetting) {
												motorSetting.mSpringSettings.mFrequency = 20;
												motorSetting.mSpringSettings.mDamping = 0;
										}
								})
								part.mToParent = constraint;
						}
				};

				// Initialize the skeleton
				ragdoll.GetSkeleton().CalculateParentJointIndices();
				// Stabilize the constraints of the ragdoll
				ragdoll.Stabilize();

				// Calculate body <-> constraint map
				ragdoll.CalculateBodyIndexToConstraintIndex();
				ragdoll.CalculateConstraintIndexToBodyIdxPair();

				Jolt.destroy(vec3);
				Jolt.destroy(vec4);
				Jolt.destroy(quat);
				return ragdoll;
		},
		loadAnimation: async function (Jolt, filename) {
				const S = (string) => new Jolt.JPHString(string, string.length);

				const text = await fetch(filename).then(res => res.text());
				const objects = ObjectStreamIn.sReadObject(text);
				const { mAnimatedJoints } = objects.result;
				const skeletalAnimation = new Jolt.SkeletalAnimation();
				const joints = skeletalAnimation.GetAnimatedJoints();
				joints.resize(mAnimatedJoints.length);
				mAnimatedJoints.forEach((_joint, i) => {
						const joint = joints.at(i);
						joint.mJointName = S(_joint.mJointName);
						joint.mKeyframes.resize(_joint.mKeyframes.length);
						_joint.mKeyframes.forEach((_frame, f) => {
								const keyFrame = joint.mKeyframes.at(f);
								keyFrame.mTime = _frame.mTime;
								keyFrame.mTranslation.Set(..._frame.mTranslation.value);
								keyFrame.mRotation.Set(..._frame.mRotation.value);
						});
				});
				return skeletalAnimation;
		}
}
