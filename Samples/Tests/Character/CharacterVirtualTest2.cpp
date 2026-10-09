// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#include <Samples.h>

#include <Tests/Character/CharacterVirtualTest2.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Layers.h>
#include <Utils/Log.h>
#include <Renderer/DebugRendererImp.h>
#include <Application/DebugUI.h>

//#define CHARACTER_TRACE_CONTACTS

JPH_IMPLEMENT_RTTI_VIRTUAL(CharacterVirtualTest2)
{
	JPH_ADD_BASE_CLASS(CharacterVirtualTest2, CharacterBaseTest)
}

void CharacterVirtualTest2::Initialize()
{
	CharacterBaseTest::Initialize();

	// Create 'player' character
	Ref<CharacterVirtualSettings> settings = new CharacterVirtualSettings();
	settings->mMaxSlopeAngle = sMaxSlopeAngle;
	settings->mMaxStrength = sMaxStrength;
	settings->mShape = mStandingShape;
	settings->mBackFaceMode = sBackFaceMode;
	settings->mCharacterPadding = sCharacterPadding;
	settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
	settings->mPredictiveContactDistance = sPredictiveContactDistance;
	//settings->mSupportingVolume = Plane(Vec3::sAxisY(), -cCharacterRadiusStanding); // Accept contacts that touch the lower sphere of the capsule
	settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
	settings->mInnerBodyShape = sCreateInnerBody? mInnerStandingShape : nullptr;
	settings->mInnerBodyLayer = Layers::MOVING;
	mCharacter = new CharacterVirtual(settings, RVec3::sZero(), Quat::sIdentity(), 0, mPhysicsSystem);
	//mCharacter->SetCharacterVsCharacterCollision(&mCharacterVsCharacterCollision);
	//mCharacterVsCharacterCollision.Add(mCharacter);

	//// Draw labels on ramp blocks
	//for (size_t i = 0; i < mRampBlocks.size(); ++i)
	//	SetBodyLabel(mRampBlocks[i], StringFormat("PushesPlayer: %s\nPushable: %s", (i & 1) != 0? "True" : "False", (i & 2) != 0? "True" : "False"));
}

void CharacterVirtualTest2::PrePhysicsUpdate(const PreUpdateParams &inParams)
{
	CharacterBaseTest::PrePhysicsUpdate(inParams);

	// Draw character pre update (the sim is also drawn pre update)
	RMat44 com = mCharacter->GetCenterOfMassTransform();
	RMat44 world_transform = mCharacter->GetWorldTransform();
#ifdef JPH_DEBUG_RENDERER
	mCharacter->GetShape()->Draw(mDebugRenderer, com, Vec3::sOne(), Color::sGreen, false, true);
#endif // JPH_DEBUG_RENDERER

	DrawPaddedCharacter(mCharacter->GetShape(), mCharacter->GetCharacterPadding(), com);

	// Remember old position
	RVec3 old_position = mCharacter->GetPosition();

	// Settings for our update function
	CharacterVirtual::ExtendedUpdateSettings update_settings;
	if (!sEnableStickToFloor)
		update_settings.mStickToFloorStepDown = Vec3::sZero();
	else
		update_settings.mStickToFloorStepDown = -mCharacter->GetUp() * update_settings.mStickToFloorStepDown.Length();
	if (!sEnableWalkStairs)
		update_settings.mWalkStairsStepUp = Vec3::sZero();
	else
		update_settings.mWalkStairsStepUp = mCharacter->GetUp() * update_settings.mWalkStairsStepUp.Length();

	// Update the character position
	mCharacter->ExtendedUpdate(inParams.mDeltaTime,
		-mCharacter->GetUp() * mPhysicsSystem->GetGravity().Length(),
		update_settings,
		mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
		mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING),
		{ },
		{ },
		*mTempAllocator);

	// Calculate effective velocity
	RVec3 new_position = mCharacter->GetPosition();
	Vec3 velocity = Vec3(new_position - old_position) / inParams.mDeltaTime;

	// Draw state of character
	DrawCharacterState(mCharacter, world_transform, velocity);
}

void CharacterVirtualTest2::HandleInput(Vec3Arg inMovementDirection, bool inJump, bool inSwitchStance, float inDeltaTime)
{
	bool player_controls_horizontal_velocity = sControlMovementDuringJump || mCharacter->IsSupported();
	if (player_controls_horizontal_velocity)
	{
		// Smooth the player input
		mDesiredVelocity = sEnableCharacterInertia? 0.25f * inMovementDirection * sCharacterSpeed + 0.75f * mDesiredVelocity : inMovementDirection * sCharacterSpeed;

		// True if the player intended to move
		mAllowSliding = !inMovementDirection.IsNearZero();
	}
	else
	{
		// While in air we allow sliding
		mAllowSliding = true;
	}

	// Update the character rotation and its up vector to match the up vector set by the user settings
	Quat character_up_rotation = Quat::sEulerAngles(Vec3(sUpRotationX, 0, sUpRotationZ));
	mCharacter->SetUp(character_up_rotation.RotateAxisY());
	mCharacter->SetRotation(character_up_rotation);

	// A cheaper way to update the character's ground velocity,
	// the platforms that the character is standing on may have changed velocity
	mCharacter->UpdateGroundVelocity();

	// Determine new basic velocity
	Vec3 current_vertical_velocity = mCharacter->GetLinearVelocity().Dot(mCharacter->GetUp()) * mCharacter->GetUp();
	Vec3 ground_velocity = mCharacter->GetGroundVelocity();
	Vec3 new_velocity;
	bool moving_towards_ground = (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f;
	if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround	// If on ground
		&& (sEnableCharacterInertia?
			moving_towards_ground													// Inertia enabled: And not moving away from ground
			: !mCharacter->IsSlopeTooSteep(mCharacter->GetGroundNormal())))			// Inertia disabled: And not on a slope that is too steep
	{
		// Assume velocity of ground when on ground
		new_velocity = ground_velocity;

		// Jump
		if (inJump && moving_towards_ground)
			new_velocity += sJumpSpeed * mCharacter->GetUp();
	}
	else
		new_velocity = current_vertical_velocity;

	// Gravity
	//new_velocity += (character_up_rotation * mPhysicsSystem->GetGravity()) * inDeltaTime;

	if (player_controls_horizontal_velocity)
	{
		// Player input
		new_velocity += character_up_rotation * mDesiredVelocity;
	}
	else
	{
		// Preserve horizontal velocity
		Vec3 current_horizontal_velocity = mCharacter->GetLinearVelocity() - current_vertical_velocity;
		new_velocity += current_horizontal_velocity;
	}

	// Update character velocity
	mCharacter->SetLinearVelocity(new_velocity);

	// Stance switch
	if (inSwitchStance)
	{
		bool is_standing = mCharacter->GetShape() == mStandingShape;
		const Shape *shape = is_standing? mCrouchingShape : mStandingShape;
		if (mCharacter->SetShape(shape, 1.5f * mPhysicsSystem->GetPhysicsSettings().mPenetrationSlop, mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING), { }, { }, *mTempAllocator))
		{
			const Shape *inner_shape = is_standing? mInnerCrouchingShape : mInnerStandingShape;
			mCharacter->SetInnerBodyShape(inner_shape);
		}
	}
}

void CharacterVirtualTest2::SaveState(StateRecorder &inStream) const
{
	CharacterBaseTest::SaveState(inStream);

	mCharacter->SaveState(inStream);

	bool is_standing = mCharacter->GetShape() == mStandingShape;
	inStream.Write(is_standing);

	inStream.Write(mAllowSliding);
	inStream.Write(mDesiredVelocity);
}

void CharacterVirtualTest2::RestoreState(StateRecorder &inStream)
{
	CharacterBaseTest::RestoreState(inStream);

	mCharacter->RestoreState(inStream);

	bool is_standing = mCharacter->GetShape() == mStandingShape; // Initialize variable for validation mode
	inStream.Read(is_standing);
	const Shape *shape = is_standing? mStandingShape : mCrouchingShape;
	mCharacter->SetShape(shape, FLT_MAX, { }, { }, { }, { }, *mTempAllocator);
	const Shape *inner_shape = is_standing? mInnerStandingShape : mInnerCrouchingShape;
	mCharacter->SetInnerBodyShape(inner_shape);

	inStream.Read(mAllowSliding);
	inStream.Read(mDesiredVelocity);
}
