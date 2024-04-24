#include "ClimbComponent.h"

#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"

TAutoConsoleVariable<bool> CVar_DebugLedgeUp(TEXT("deft.debug.climb.ledgeup"), false, TEXT("draw debugging for ledgeup"), ECVF_Cheat);

UClimbComponent::UClimbComponent()
	: DeftCharacter(nullptr)
	, DeftMovementComponent(nullptr)
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
		LedgeReachDistance = 10.f;
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftPlayerCharacter!"));
}

void UClimbComponent::LedgeUp()
{
	// we're in a jump or fall state
	const bool isInAir = DeftMovementComponent->IsDeftJumping() || DeftMovementComponent->IsDeftFalling();
	if (!isInAir)
		return;

	const float capsuleEdgeDistance = DeftCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius();
	const FVector actorFwd = DeftCharacter->GetActorForwardVector().GetSafeNormal();

	// check if 'something' is in range


	// check if there is no obstacle infront of us at least wide enough to fit on
	const FVector ledgeTraceStart = DeftCharacter->GetActorLocation() + FVector(0.f, 0.f, LedgeHeightMin) + (actorFwd * capsuleEdgeDistance);
	const FVector ledgeTraceEnd = ledgeTraceStart + (actorFwd.GetSafeNormal() * LedgeWidthRequirement);

	FCollisionQueryParams queryParams;
	queryParams.AddIgnoredActor(DeftCharacter.Get());

	FHitResult hit;
	bool isBlockingHit = GetWorld()->LineTraceSingleByChannel(hit, ledgeTraceStart, ledgeTraceEnd, ECC_WorldStatic, queryParams);
	if (isBlockingHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Can't ledge up: no ledge in range or not wide enough"));
		return;
	}

	// check that it's not a drop off and/or not walkable
	const FVector downLedgeTrace = ledgeTraceEnd + (FVector::DownVector * LedgeHeightMin);
	isBlockingHit = GetWorld()->LineTraceSingleByChannel(hit, ledgeTraceEnd, downLedgeTrace, ECC_WorldStatic, queryParams);
	if (!isBlockingHit)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, TEXT("Can't ledge up: nothing to stand on"));
		return;
	}
	// TODO: check the surface normal to make sure it's physically walkable
	

	// there is room on the ledge for us 
	
			// after eye height check, go that distance out, then down to find floor.
			// then shape sweep at floor location for our capsul size to see if we hit anything

	// enter ledge up state

#if !UE_BUILD_SHIPPING
	Debug_LedgeTraceStart = ledgeTraceStart;
	Debug_LedgeTraceEnd = ledgeTraceEnd;

	Debug_LedgeSurfaceTraceStart = ledgeTraceEnd;
	Debug_LedgeSurfaceTraceEnd = downLedgeTrace;
#endif // !UE_BUILD_SHIPPING
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

	DrawDebugLine(GetWorld(), Debug_LedgeTraceStart, Debug_LedgeTraceEnd, FColor::Green);
	DrawDebugLine(GetWorld(), Debug_LedgeSurfaceTraceStart, Debug_LedgeSurfaceTraceEnd, FColor::Green);
}

#endif // !UE_BUILD_SHIPPING

