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

private:
	bool IsLedgeReachable();
	bool IsLedgeWithinHeightRange(FVector& outHeightDistanceTraceEnd);
	bool IsLedgeSurfaceWalkable(const FVector& aHeightDistanceTraceEnd, FHitResult& outSurfaceHit);
	bool IsEnoughRoomOnLedge(const FHitResult& aSurfaceHit);

	FCollisionQueryParams CollisionQueryParams;
	FCollisionShape CapsuleCollisionShape;

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;
	TWeakObjectPtr<class UDeftCharacterMovementComponent> DeftMovementComponent;

	float CapsuleRadius;
	float LedgeHeightMin;
	float LedgeWidthRequirement;
	float LedgeReachDistance; // how far away we can be from a ledge for it to activate

#if !UE_BUILD_SHIPPING
	void DrawDebug();
	void DrawDebugLedgeUp();

	bool Debug_LedgeReach;
	FVector Debug_LedgeReachLoc;
	FColor Debug_LedgeReachColor;

	bool Debug_LedgeHeight;
	FVector Debug_LedgeHeightStart;
	FVector Debug_LedgeHeightEnd;
	FColor Debug_LedgeHeightColor;
	
	bool Debug_LedgeSurface;
	FVector Debug_LedgeSurfaceStart;
	FVector Debug_LedgeSurfaceEnd;
	FColor Debug_LedgeSurfaceColor;

	bool Debug_LedgeWidth;
	FVector Debug_LedgeWidthLoc;
	FColor Debug_LedgeWidthColor;
#endif // !UE_BUILD_SHIPPING
};
