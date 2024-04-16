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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta=(DisplayName="Jump Land Dip Curve"))
	UCurveFloat* JumpLandDipCuve;

private:
	void ProcessCameraBobble(float aDeltaTime);
	void ProcessCameraRoll(float aDeltaTime);
	void PerformCameraLandDip();

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftPlayerCharacter;
	TWeakObjectPtr<class USceneComponent> BobbleTarget;

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
	bool bUnrollFromLeft;
	bool bNeedsUnroll;

	bool bIsWalkBobbleActive;
};
