// Jolt Physics Library (https://github.com/jrouwe/JoltPhysics)
// SPDX-FileCopyrightText: 2021 Jorrit Rouwe
// SPDX-License-Identifier: MIT

#pragma once

#include <Tests/Character/CharacterBaseTest.h>

class CharacterVirtualTest2 : public CharacterBaseTest
{
public:
	JPH_DECLARE_RTTI_VIRTUAL(JPH_NO_EXPORT, CharacterVirtualTest2)

	// Description of the test
	virtual const char *	GetDescription() const override
	{
		return "Shows the CharacterVirtual class. Move around with the arrow keys, Shift for crouch and Ctrl for jump.";
	}

	// Initialize the test
	virtual void			Initialize() override;

	// Update the test, called before the physics update
	virtual void			PrePhysicsUpdate(const PreUpdateParams &inParams) override;

	// Saving / restoring state for replay
	virtual void			SaveState(StateRecorder &inStream) const override;
	virtual void			RestoreState(StateRecorder &inStream) override;

	// Get position of the character
	virtual RVec3			GetCharacterPosition() const override				{ return mCharacter->GetPosition(); }

	// Handle user input to the character
	virtual void			HandleInput(Vec3Arg inMovementDirection, bool inJump, bool inSwitchStance, float inDeltaTime) override;

private:
	// Character movement settings
	static inline bool		sEnableCharacterInertia = true;

	// Test configuration settings
	static inline EBackFaceMode sBackFaceMode = EBackFaceMode::CollideWithBackFaces;
	static inline float		sUpRotationX = 0;
	static inline float		sUpRotationZ = 0;
	static inline float		sMaxSlopeAngle = DegreesToRadians(45.0f);
	static inline float		sMaxStrength = 100.0f;
	static inline float		sCharacterPadding = 0.0000001f;
	static inline float		sPenetrationRecoverySpeed = 1.0f;
	static inline float		sPredictiveContactDistance = 0.1f;
	static inline bool		sEnableWalkStairs = true;
	static inline bool		sEnableStickToFloor = true;
	static inline bool		sEnhancedInternalEdgeRemoval = false;
	static inline bool		sCreateInnerBody = false;
	static inline bool		sPlayerCanPushOtherCharacters = true;
	static inline bool		sOtherCharactersCanPushPlayer = true;

	// The 'player' character
	Ref<CharacterVirtual>	mCharacter;

	// Smoothed value of the player input
	Vec3					mDesiredVelocity = Vec3::sZero();

	// True when the player is pressing movement controls
	bool					mAllowSliding = false;
};
