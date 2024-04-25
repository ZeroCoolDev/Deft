#include "ClimbComponent.h"

#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"

TAutoConsoleVariable<bool> CVar_DebugLedgeUp(TEXT("deft.debug.climb.ledgeup"), false, TEXT("draw debugging for ledgeup"), ECVF_Cheat);

UClimbComponent::UClimbComponent()
	: CollisionQueryParams()
	, CapsuleCollisionShape()
	, DeftCharacter(nullptr)
	, DeftMovementComponent(nullptr)
	, CapsuleRadius(0.f)
	, LedgeHeightMin(0.f)
	, LedgeWidthRequirement(0.f)
	, LedgeReachDistance(0.f)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	DrawDebug();
#endif // !UE_BUILD_SHIPPING
}


void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	DeftCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (DeftCharacter.IsValid())
	{
		CollisionQueryParams.AddIgnoredActor(DeftCharacter.Get());
		const UCapsuleComponent* capsulComponent = DeftCharacter->GetCapsuleComponent();
		CapsuleCollisionShape = FCollisionShape::MakeCapsule(capsulComponent->GetUnscaledCapsuleRadius(), capsulComponent->GetUnscaledCapsuleHalfHeight());

		DeftMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftCharacter->GetCharacterMovement());

		DeftCharacter->OnJumpInputPressed.AddUObject(this, &UClimbComponent::LedgeUp);

		if (USpringArmComponent* springArmComponent = DeftCharacter->FindComponentByClass<USpringArmComponent>())
			LedgeHeightMin = springArmComponent->GetComponentLocation().Z - DeftCharacter->GetActorLocation().Z;
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed finding the spring arm component, defaulting to UE eye height"));
			LedgeHeightMin = DeftCharacter->BaseEyeHeight;
		}

		// the ledge must be wide enough to fit the player
		LedgeWidthRequirement = DeftCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * 2;

		// how far away can we be from the ledge for the ledge-up to activate (needs to be pretty darn close I imagine)
		LedgeReachDistance = 50.f;
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftPlayerCharacter!"));
}

void UClimbComponent::LedgeUp()
{
#if !UE_BUILD_SHIPPING
	Debug_LedgeReach = false;
	Debug_LedgeHeight= false;
	Debug_LedgeSurface = false;
	Debug_LedgeWidth = false;
#endif //!UE_BUILD_SHIPPING

	// we're in a jump or fall state
	const bool isInAir = DeftMovementComponent->IsDeftJumping() || DeftMovementComponent->IsDeftFalling();
	if (!isInAir)
		return;

#if !UE_BUILD_SHIPPING
	Debug_LedgeReach = true;
#endif //!UE_BUILD_SHIPPING
	if (!IsLedgeReachable())
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Can't ledge up: nothing in reach"));
		return;
	}

#if !UE_BUILD_SHIPPING
	Debug_LedgeHeight = true;
#endif //!UE_BUILD_SHIPPING
	FVector heightDistanceTraceEnd;
	if (!IsLedgeWithinHeightRange(heightDistanceTraceEnd))
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Can't ledge up: no ledge or obstacle obstructing"));
		return;
	}

#if !UE_BUILD_SHIPPING
	Debug_LedgeSurface = true;
#endif //!UE_BUILD_SHIPPING
	FHitResult surfaceHit;
	if (!IsLedgeSurfaceWalkable(heightDistanceTraceEnd, surfaceHit))
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Can't ledge up: nothing to stand on"));
		return;
	}

#if !UE_BUILD_SHIPPING
	Debug_LedgeWidth = true;
#endif //!UE_BUILD_SHIPPING
	if (!IsEnoughRoomOnLedge(surfaceHit))
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Can't ledge up: obstacles in the way on ledge"));
		return;
	}

	GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, TEXT("Ledge has been detected!"));

	// there is room on the ledge for us 
	
			// after eye height check, go that distance out, then down to find floor.
			// then shape sweep at floor location for our capsul size to see if we hit anything

	// enter ledge up state
}

bool UClimbComponent::IsLedgeReachable()
{
	// check if 'something' is in range
	const FVector ledgeReachStart = DeftCharacter->GetActorLocation();
	const FVector ledgeReachEnd = ledgeReachStart + (DeftCharacter->GetActorForwardVector().GetSafeNormal() * LedgeReachDistance);
	FHitResult reachHit;

	const bool isBlockingHit = GetWorld()->SweepSingleByProfile(reachHit, ledgeReachStart, ledgeReachEnd, DeftCharacter->GetActorRotation().Quaternion(), DeftCharacter->GetCapsuleComponent()->GetCollisionProfileName(), CapsuleCollisionShape, CollisionQueryParams);
#if !UE_BUILD_SHIPPING
	Debug_LedgeReachLoc = isBlockingHit ? reachHit.Location : ledgeReachEnd;
	Debug_LedgeReachColor = isBlockingHit ? FColor::Green : FColor::Red;
#endif //!UE_BUILD_SHIPPING
	return isBlockingHit;
}

bool UClimbComponent::IsLedgeWithinHeightRange(FVector& outHeightDistanceTraceEnd)
{
	// check if a "ledge" exists i.e. open space above a surface wide enough to stand on
	const FVector actorFwdNormal = DeftCharacter->GetActorForwardVector().GetSafeNormal();
	const FVector ledgeTraceStart = DeftCharacter->GetActorLocation() + FVector(0.f, 0.f, LedgeHeightMin) + (actorFwdNormal * CapsuleRadius);
	outHeightDistanceTraceEnd = ledgeTraceStart + (actorFwdNormal * LedgeWidthRequirement);
	FHitResult ledgeHeightHit;

	const bool isBlockingHit = GetWorld()->LineTraceSingleByChannel(ledgeHeightHit, ledgeTraceStart, outHeightDistanceTraceEnd, ECC_WorldStatic, CollisionQueryParams);
	if (isBlockingHit)
		outHeightDistanceTraceEnd = ledgeHeightHit.Location;
#if !UE_BUILD_SHIPPING
	Debug_LedgeHeightStart = ledgeTraceStart;
	Debug_LedgeHeightEnd = outHeightDistanceTraceEnd;
	Debug_LedgeHeightColor = isBlockingHit ? FColor::Red : FColor::Green;
#endif //!UE_BUILD_SHIPPING
	return !isBlockingHit;
}

bool UClimbComponent::IsLedgeSurfaceWalkable(const FVector& aHeightDistanceTraceEnd, FHitResult& outSurfaceHit)
{
	// check that it's not a drop off and/or not walkable
	const FVector surfaceTraceEnd = aHeightDistanceTraceEnd + (FVector::DownVector * LedgeHeightMin * 2.f);

	const bool isBlockingHit = GetWorld()->LineTraceSingleByChannel(outSurfaceHit, aHeightDistanceTraceEnd, surfaceTraceEnd, ECC_WorldStatic, CollisionQueryParams);
#if !UE_BUILD_SHIPPING
	Debug_LedgeSurfaceStart = aHeightDistanceTraceEnd;
	Debug_LedgeSurfaceEnd = isBlockingHit ? outSurfaceHit.Location : surfaceTraceEnd;
	Debug_LedgeSurfaceColor = isBlockingHit ? FColor::Green : FColor::Red;
#endif // !UE_BUILD_SHIPPING
	return isBlockingHit;

	// TODO: check the surface normal to make sure it's physically walkable
}

bool UClimbComponent::IsEnoughRoomOnLedge(const FHitResult& aSurfaceHit)
{
	// check that there is enough space on the ledge for the player's capsule component
	const FVector widthStart = aSurfaceHit.Location								// Start at the surface collision 
		+ (FVector::UpVector * CapsuleCollisionShape.GetCapsuleHalfHeight());	// raise it by half the height of the capsule since the origin is in the middle

	const FVector widthEnd = widthStart - DeftCharacter->GetActorForwardVector().GetSafeNormal(); // making the end a very small distance _closer_ to the player because of an issue with UE you can't sweep a shape literally in the exact same location
	FHitResult ledgeWidthHit;
	FCollisionQueryParams excludeSurfaceActor;
	excludeSurfaceActor.AddIgnoredActor(DeftCharacter.Get());
	excludeSurfaceActor.AddIgnoredActor(aSurfaceHit.GetActor());

	const bool isBlockingHit = GetWorld()->SweepSingleByProfile(ledgeWidthHit, widthStart, widthEnd, DeftCharacter->GetActorRotation().Quaternion(), DeftCharacter->GetCapsuleComponent()->GetCollisionProfileName(), CapsuleCollisionShape, excludeSurfaceActor);
#if !UE_BUILD_SHIPPING
	Debug_LedgeWidthLoc = isBlockingHit ? ledgeWidthHit.Location : widthStart;
	Debug_LedgeWidthColor = isBlockingHit ? FColor::Red : FColor::Green;
#endif // !UE_BUILD_SHIPPING
	return !isBlockingHit;
}

#if !UE_BUILD_SHIPPING
void UClimbComponent::DrawDebug()
{
	if (CVar_DebugLedgeUp.GetValueOnGameThread())
		DrawDebugLedgeUp();
}

void UClimbComponent::DrawDebugLedgeUp()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::Yellow, FString::Printf(TEXT("\n-Ledge Up-\n\tHeight Req.: %.2f\n\tWidth Req.: %.2f\n\tReach Dist: %.2f"), LedgeHeightMin, LedgeWidthRequirement, LedgeReachDistance));

	// debug reach
	if (Debug_LedgeReach)
		DrawDebugCapsule(GetWorld(), Debug_LedgeReachLoc, CapsuleCollisionShape.GetCapsuleHalfHeight(), CapsuleCollisionShape.GetCapsuleRadius(), DeftCharacter->GetActorRotation().Quaternion(), Debug_LedgeReachColor);
	
	// debug height
	if (Debug_LedgeHeight)
		DrawDebugLine(GetWorld(), Debug_LedgeHeightStart, Debug_LedgeHeightEnd, Debug_LedgeHeightColor);
	
	// debug surface
	if (Debug_LedgeSurface)
		DrawDebugLine(GetWorld(), Debug_LedgeSurfaceStart, Debug_LedgeSurfaceEnd, Debug_LedgeSurfaceColor);
	
	// debug reach
	if (Debug_LedgeWidth)
		DrawDebugCapsule(GetWorld(), Debug_LedgeWidthLoc, CapsuleCollisionShape.GetCapsuleHalfHeight(), CapsuleCollisionShape.GetCapsuleRadius(), DeftCharacter->GetActorRotation().Quaternion(), Debug_LedgeWidthColor);
}

#endif // !UE_BUILD_SHIPPING

