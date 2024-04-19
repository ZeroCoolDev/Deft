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

	bool DoJump(bool bReplayingMoves) override;
	bool IsFalling() const override;
	bool CanAttemptJump() const override;
	bool CanCrouchInCurrentState() const override;
	void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Jump Curve"))
	UCurveFloat* JumpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Fall Curve"))
	UCurveFloat* FallCurve;

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
	bool bIsValidJumpCurve;
	bool bIsFalling;
	bool bIsSliding;

private:
#if !UE_BUILD_SHIPPING
	float JumpHeightApexTest = 0.f;
	bool IsJumpCurveEnabled() const;
#endif// UE_BUILD_SHIPPING
};
