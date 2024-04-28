// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraMovementComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UCameraMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UCameraMovementComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves", meta=(DisplayName="Walk Bobble Curve"))
	UCurveFloat* WalkBobbleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curves", meta=(DisplayName="Landed From Air Dip Curve"))
	UCurveFloat* LandedFromAirDipCuve;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float DefaultCameraRelativeZPosition;

private:
	void ProcessCameraBobble(float aDeltaTime);

	void ProcessCameraRoll(float aDeltaTime);
	void PreRoll();
	void Roll(float aDeltaTime, bool aIsLeaningLeft);
	void PreUnRoll(bool aWasLeaningLeft);
	void UnRoll(float aDeltaTime);

	void ProcessCameraDip(float aDeltaTime);

	void ProcessCameraPitch(float aDeltaTime);
	void PrePitch();
	void PreUnPitch();

	void ProcessCameraSlide(float aDeltaTime);
	void EnterSlide();
	void UnSlide();
	void ExitSlide();

	void OnLandedFromAir();
	void OnSlideActionOccured(bool aIsSlideActive);

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;
	TWeakObjectPtr<class UDeftCharacterMovementComponent> DeftMovementComponent;
	TWeakObjectPtr<class USceneComponent> CameraTarget;

	FVector2D PreviousInputVector;

	// Bobble
	float WalkBobbleTime;
	float WalkBobbleMaxTime;
	float PrevWalkBobbleVal;

	// Roll/Unroll
	float RollLerpTimeMax;
	float RollLerpTime;
	float RollLerpStart;
	float RollLerpEnd;
	float HighestRollTimeAchieved;
	float UnrollLerpTime;
	float UnrollLerpTimeMax;

	// Dip
	float DipLerpTimeMax;
	float DipLerpTime;
	float PrevDipVal;

	// Back Tilt
	float PitchLerpTime;
	float PitchLerpTimeMax;
	float PitchStart;
	float PitchEnd;
	float PrevPitch;
	float HighestTiltTimeAchieved;
	float UnPitchLerpTime;
	float UnPitchLerpTimeMax;
	float PrevUnPitch;

	// Slide
	float SlideZPosLerpTime;
	float SlideZPosLerpTimeMax;
	float SlideZPosStart;
	float SlideZPosEnd;
	float PrevSlideZPos;
	float HighestSlideZPosTimeAchieved;
	// TODO: not stoked about this, but it allows code reuse so thats cool
	float SlideRollEndOverride;
	float SlideRollLerpTimeMaxOverride;
	
	bool bIsSlideActive;
	bool bIsUnSlideActive;

	bool bIsPitchActive;
	bool bIsUnPitchActive;

	bool bNeedsUnroll;
	bool bUnrollFromLeft;

	bool bNeedsDip;
	
#if !UE_BUILD_SHIPPING
	void DrawDebug();
	void DrawDebugBobble();
	void DrawDebugRoll();
	void DrawDebugDip();
	void DrawDebugPitch();
	void DrawDebugSlide();

	bool bIsLeaningRight;
	bool bIsLeaningLeft;
	bool bShouldStopBobble;
#endif//!UE_BUILD_SHIPPING
};
