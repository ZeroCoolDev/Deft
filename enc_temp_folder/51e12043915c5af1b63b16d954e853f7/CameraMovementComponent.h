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


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bobble", meta=(DisplayName="Walk Bobble Curve"))
	UCurveFloat* WalkBobbleCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta=(DisplayName="Landed From Air Dip Curve"))
	UCurveFloat* LandedFromAirDipCuve;

private:
	void ProcessCameraBobble(float aDeltaTime);
	void ProcessCameraRoll(float aDeltaTime);
	void ProcessCameraDip(float aDeltaTime);
	void BeginCameraLandDip();

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftPlayerCharacter;
	TWeakObjectPtr<class USceneComponent> CameraTarget;

	FVector2D PreviousInputVector;

	float WalkBobbleTime;
	float WalkBobbleMaxTime;
	float PrevWalkBobbleVal;

	float RollLerpTimeMax;
	float RollLerpTime;
	float RollLerpStart;
	float RollLerpEnd;
	float HighestRollTimeAchieved;

	float UnrollLerpTime;
	float UnrollLerpTimeMax;

	float DipLerpTimeMax;
	float DipLerpTime;
	float PrevDipVal;

	bool bNeedsUnroll;
	bool bUnrollFromLeft;

	bool bNeedsDip;
	
	bool bIsWalkBobbleActive;
};
