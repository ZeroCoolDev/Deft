#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DeftCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class DEFT_API UDeftCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UDeftCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

protected:
	void BeginPlay() override;
	void TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction) override;
	bool DoJump(bool bReplayingMoves) override;
	bool IsFalling() const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement | Jump", meta=(DisplayName="Jump Curve"))
	UCurveFloat* JumpCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement | Jump", meta=(DisplayName="Jump Fall Curve"))
	UCurveFloat* JumpFallCurve;

private:
	void ProcessJump(float aDeltaTime);
	void ProcessJumpFall(float aDeltaTime);
	
	void SetCustomFallingMode();
	bool CustomFindFloor(FFindFloorResult& outFloorResult, const FVector aStartLoc, const FVector aEndLoc);
	bool FindJumpApexTime(float& outApexTime);

	bool bIsJumping;			// true while character is following the jump curve
	float JumpTime;				// amount of time elapsed from the jump start
	float PrevJumpTime;
	float PrevJumpCurveVal;
	//  time bounds
	float JumpCurveMinTime;
	float JumpCurveMaxTime;

	// TODO: falling is custom for everything not just jump so remove 'jump' from names
	bool bIsJumpFalling;
	float JumpFallTime;
	float PrevJumpFallCurveVal;

	bool bIsValidJumpCurve;
	float JumpApexTime;
	float JumpApexHeight;

private:
#if !UE_BUILD_SHIPPING
	float JumpHeightApexTest = 0.f;
	bool IsJumpCurveEnabled() const;
#endif// UE_BUILD_SHIPPING
};
