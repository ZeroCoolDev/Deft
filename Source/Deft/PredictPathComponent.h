// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PredictPathComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UPredictPathComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UPredictPathComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	bool PredictPath_Parabola(float aSpeed, float aAngle, const FVector& aDir, const FVector& aPathEnd, TArray<FVector>& outPath);

private:
	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;

	//TODO: macro debug these
	TArray<FVector> PredictedPathPoints;
	FVector PredictedDir;
	FVector PredictedOrigin;
	FVector PredictedEnd;

	void DebugDraw();
};