// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UClimbComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UClimbComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void LedgeUp();

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;
	TWeakObjectPtr<class UDeftCharacterMovementComponent> DeftMovementComponent;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Ledge Up")
	float LedgeHeightMin;
	float LedgeWidthRequirement;
	float LedgeReachDistance; // how far away we can be from a ledge for it to activate

#if !UE_BUILD_SHIPPING
	void DrawDebug();
	void DrawDebugLedgeUp();

	FVector Debug_LedgeTraceStart;
	FVector Debug_LedgeTraceEnd;
	FVector Debug_LedgeSurfaceTraceStart;
	FVector Debug_LedgeSurfaceTraceEnd;
#endif // !UE_BUILD_SHIPPING
};