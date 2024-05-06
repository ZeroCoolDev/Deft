// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrajectoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UTrajectoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTrajectoryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Init(float aSpeed, float aAngle);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void CalculateTrajectory();
	AActor* Owner;

private:
	FVector Velocity;
	float Angle;
};
