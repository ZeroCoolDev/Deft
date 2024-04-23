#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DeftCharacterMovementComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FLandedFromAirDelegate);
DECLARE_MULTICAST_DELEGATE_OneParam(FSlideActionOccurredDelegate, bool /*aIsSlidingActive*/);

/**
 * 
 */
UCLASS()
class DEFT_API UDeftCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UDeftCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

	FLandedFromAirDelegate OnLandedFromAir;
	FSlideActionOccurredDelegate OnSlideActionOccured;

	void DoSlide();

protected:
	void BeginPlay() override;
	void TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction) override;

	// Override Reason: Custom jump logic using curves and not gravity x velocity
	bool DoJump(bool bReplayingMoves) override;

	// Override Reason: default engine IsFalling() for animation reasons since we logically are in MOVE_Flying during any Jump or Fall however animations may need to know if we're falling
	bool IsFalling() const override;

	// Override Reason: We want to allow jumping while sliding
	bool CanAttemptJump() const override;

	// Override reason: Slide puts us in MOVE_Flying which is excluded from crouch allowance, so we need to check if we're sliding
	bool CanCrouchInCurrentState() const override;

	// Override reason: Forcing crouch to maintain base location while in MOVE_Flying since that's our custom slide and we want the capsule to stay at ground level
	// otherwise base UE implementation sets bCrouchMaintainsBaseLocation = false whenever we're MOVE_Flying
	void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	
	// Override reason: This will trigger during jumping/falling with horizontal collision and cause a bug
	// overriding to exclude step-up checks while jumping or falling.
	// (context) Step-Up is for small collisions in the velocity direction which allows automatic traversal "up" the "step" (think shallow stairs)
	bool CanStepUp(const FHitResult& Hit) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Jump Curve"))
	UCurveFloat* JumpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Jump Fall Curve"))
	UCurveFloat* JumpFallCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Non Jump Fall Curve"))
	UCurveFloat* NonJumpFallCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Slide Curve"))
	UCurveFloat* SlideCurve;

private:
	void ProcessJumping(float aDeltaTime);
	void ProcessFalling(float aDeltaTime);
	void ProcessSliding(float aDeltaTime);

	void StopSlide();

	void SetCustomFallingMode();
	bool FindFloorBySweep(FFindFloorResult& outFloorResult, const FVector aStartLoc, const FVector aEndLWoc);
	bool FindJumpApexTime(float& outApexTime);

	FVector SlideDirection;
	FVector SlideJumpAdditive;

	UCurveFloat* FallCurveToUse;

	// Jumping
	float JumpTime;
	float PrevJumpTime;
	float JumpCurveStartTime;
	float JumpCurveMaxTime;
	float PrevJumpCurveVal;
	float JumpApexTime;
	float JumpApexHeight;

	// Falling
	float FallTime;
	float PrevFallCurveVal;

	// Slide
	float SlideTime;
	float SlideCurveStartTime;
	float SlideCurveMaxTime;
	float SlideMinimumStartTime;		// (deprecated) Slide will default to a minimum duration if velocity is very low (i.e. player barely taps a direction and slides)
	float SlideJumpSpeedMod;			// Jump Speed modifier based off slide speed to give the player a longer jump during slide
	float SlideJumpSpeedModMax;

	bool bIsJumping;
	bool bWasJumpingLastFrame;
	bool bIsValidJumpCurve;
	bool bIsFalling;
	bool bIsSliding;

private:
#if !UE_BUILD_SHIPPING
	float JumpHeightApexTest = 0.f;
	bool IsJumpCurveEnabled() const;
	void DrawDebug();
#endif// UE_BUILD_SHIPPING
};
