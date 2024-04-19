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

	void ProcessCameraSlidePose(float aDeltaTime);
	void EnterSlidePose();
	void ExitSlidePose();

	void OnLandedFromAir();
	void OnSlideActionOccured(bool aIsSlideActive);

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftPlayerCharacter;
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

	float SlidePoseLerpTime;
	float SlidePoseLerpTimeMax;
	float SlidePoseStart;
	float SlidePoseEnd;
	float PrevSlidePose;
	// TODO: not stoked about this, but it allows code reuse so thats cool
	float SlidePoseRollEndOverride;
	float SlidePoseRollLerpTimeMaxOverride;
	
	bool bIsSlidePoseActive;
	bool bIsUnSlidePoseActive;

	bool bIsPitchActive;
	bool bIsUnPitchActive;

	bool bNeedsUnroll;
	bool bUnrollFromLeft;

	bool bNeedsDip;
	
	bool bIsWalkBobbleActive;
};