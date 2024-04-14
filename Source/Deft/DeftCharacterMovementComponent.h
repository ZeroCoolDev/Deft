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

protected:
	void BeginPlay() override;
	void TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction) override;
	bool DoJump(bool bReplayingMoves) override;
	bool IsFalling() const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Deft Movement", meta=(DisplayName="Jump Curve"))
	UCurveFloat* JumpCurve;

private:
#if !UE_BUILD_SHIPPING
	bool IsJumpCurveEnabled() const;
#endif// UE_BUILD_SHIPPING

	bool bIsJumping;			// true while character is following the jump curve
	float JumpTime;				// amount of time elapsed from the jump start
	float PrevJumpCurveVal;
	
	//  time bounds
	float JumpCurveMinTime;
	float JumpCurveMaxTime;
};
