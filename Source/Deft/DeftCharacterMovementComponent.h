#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DeftCharacterMovementComponent.generated.h"

DECLARE_MULTICAST_DELEGATE(FLandedFromAirDelegate);

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

protected:
	void BeginPlay() override;
	void TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction) override;
	bool DoJump(bool bReplayingMoves) override;
	bool IsFalling() const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Jump Curve"))
	UCurveFloat* JumpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Fall Curve"))
	UCurveFloat* FallCurve;

private:
	void ProcessJumping(float aDeltaTime);
	
	void ProcessFalling(float aDeltaTime);
	
	void SetCustomFallingMode();
	bool FindFloorBySweep(FFindFloorResult& outFloorResult, const FVector aStartLoc, const FVector aEndLoc);
	bool FindJumpApexTime(float& outApexTime);

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

	bool bIsJumping;
	bool bIsValidJumpCurve;
	bool bIsFalling;

private:
#if !UE_BUILD_SHIPPING
	float JumpHeightApexTest = 0.f;
	bool IsJumpCurveEnabled() const;
#endif// UE_BUILD_SHIPPING
};
