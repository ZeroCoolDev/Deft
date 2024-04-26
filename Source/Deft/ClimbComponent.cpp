#include "ClimbComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"

TAutoConsoleVariable<bool> CVar_DebugLedgeUp(TEXT("deft.debug.climb.ledgeup"), false, TEXT("draw debugging for ledgeup"), ECVF_Cheat);

UClimbComponent::UClimbComponent()
	: OnLedgeUpDelegate()
	, LedgeUpHeightBoostCurve(nullptr)
	, CollisionQueryParams()
	, CapsuleCollisionShape()
	, LedgeUpFinalLocation(FVector::ZeroVector)
	, LedgeUpStartLocation(FVector::ZeroVector)
	, DeftCharacter(nullptr)
	, DeftMovementComponent(nullptr)
	, CapsuleRadius(0.f)
	, LedgeHeightMin(0.f)
	, LedgeWidthRequirement(0.f)
	, LedgeReachDistance(0.f)
	, LedgeUpLerpTime(0.f)
	, LedgeUpLerpTimeMax(0.f)
	, LedgeUpHeightBoostMax(0.f)
	, bIsLedgeUpActive(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessLedgeUp(DeltaTime);

#if !UE_BUILD_SHIPPING
	DrawDebug();
#endif // !UE_BUILD_SHIPPING
}


void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	DeftCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (!DeftCharacter.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftCharacter"));
		return;
	}

	DeftMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftCharacter->GetCharacterMovement());
	if (!DeftMovementComponent.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftCharacterMovementComponent"));
		return;
	}

	CollisionQueryParams.AddIgnoredActor(DeftCharacter.Get());
	const UCapsuleComponent* capsulComponent = DeftCharacter->GetCapsuleComponent();
	CapsuleCollisionShape = FCollisionShape::MakeCapsule(capsulComponent->GetUnscaledCapsuleRadius(), capsulComponent->GetUnscaledCapsuleHalfHeight());

	// Ledge-up Setup
	DeftCharacter->OnJumpInputPressed.AddUObject(this, &UClimbComponent::LedgeUp);

	if (USpringArmComponent* springArmComponent = DeftCharacter->FindComponentByClass<USpringArmComponent>())
	{
		LedgeHeightMin = springArmComponent->GetComponentLocation().Z - DeftCharacter->GetActorLocation().Z + 20.f;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed finding the spring arm component, defaulting to UE eye height"));
		LedgeHeightMin = DeftCharacter->BaseEyeHeight;
	}

	// the ledge must be wide enough to fit the player
	LedgeWidthRequirement = DeftCharacter->GetCapsuleComponent()->GetUnscaledCapsuleRadius() * 2;

	// how far away can we be from the ledge for the ledge-up to activate (needs to be pretty darn close I imagine)
	LedgeReachDistance = 50.f;

	if (LedgeUpHeightBoostCurve)
	{
		float minUnused;
		LedgeUpHeightBoostCurve->GetTimeRange(minUnused, LedgeUpLerpTimeMax);
		LedgeUpHeightBoostMax = 75.f;
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Missing LedgeUpHeightBoostCurve!"));
}

void UClimbComponent::ProcessLedgeUp(float aDeltaTime)
{
	if (!bIsLedgeUpActive)
		return;

	if (LedgeUpLerpTime >= LedgeUpLerpTimeMax)
	{
		bIsLedgeUpActive = false;
		OnLedgeUpDelegate.Broadcast(bIsLedgeUpActive);
		return;
	}

	LedgeUpLerpTime += aDeltaTime;
	LedgeUpLerpTime = FMath::Clamp(LedgeUpLerpTime, 0.f, LedgeUpLerpTimeMax);

	// lerp!
	const float heightBoost = LedgeUpHeightBoostCurve->GetFloatValue(LedgeUpLerpTime) * LedgeUpHeightBoostMax;
	const float percent = LedgeUpLerpTime / LedgeUpLerpTimeMax;
	FVector ledgeUpLoc = FMath::Lerp(LedgeUpStartLocation, LedgeUpFinalLocation, percent) + (FVector::UpVector * heightBoost);

	UE_LOG(LogTemp, Warning, TEXT("heightBoost %.2f"), heightBoost);

	FLatentActionInfo latentInfo;
	latentInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo((USceneComponent*)DeftCharacter->GetCapsuleComponent(), ledgeUpLoc, DeftCharacter->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);;
}

void UClimbComponent::LedgeUp()
{
#if !UE_BUILD_SHIPPING
	Debug_LedgeReach = false;
	Debug_LedgeHeight= false;
	Debug_LedgeSurface = false;
	Debug_LedgeWidth = false;
	Debug_LedgeUpSuccess = false;
	Debug_LedgeUpMessage = "";
#endif //!UE_BUILD_SHIPPING

	// we're in a jump or fall state
	const bool isInAir = DeftMovementComponent->IsDeftJumping() || DeftMovementComponent->IsDeftFalling();
	if (!isInAir)
		return;

	if (!IsLedgeReachable())
		return;

	FVector heightDistanceTraceEnd;
	if (!IsLedgeWithinHeightRange(heightDistanceTraceEnd))
		return;

	FHitResult surfaceHit;
	if (!IsLedgeSurfaceWalkable(heightDistanceTraceEnd, surfaceHit))
		return;

	if (!IsEnoughRoomOnLedge(surfaceHit, LedgeUpFinalLocation))
		return;

#if !UE_BUILD_SHIPPING
	Debug_LedgeUpMessage = "Ledge has been detected!";
	Debug_LedgeUpSuccess = true;
#endif //!UE_BUILD_SHIPPING

	LedgeUpStartLocation = DeftCharacter->GetActorLocation();

	// enter ledge up state
	LedgeUpLerpTime = 0.f;
	bIsLedgeUpActive = true;
	OnLedgeUpDelegate.Broadcast(bIsLedgeUpActive);
}

bool UClimbComponent::IsLedgeReachable()
{
#if !UE_BUILD_SHIPPING
	Debug_LedgeReach = true;
#endif //!UE_BUILD_SHIPPING

	// check if 'something' is in range
	const FVector ledgeReachStart = DeftCharacter->GetActorLocation();
	const FVector ledgeReachEnd = ledgeReachStart + (DeftCharacter->GetActorForwardVector().GetSafeNormal() * LedgeReachDistance);
	FHitResult reachHit;

	const bool isBlockingHit = GetWorld()->SweepSingleByProfile(reachHit, ledgeReachStart, ledgeReachEnd, DeftCharacter->GetActorRotation().Quaternion(), DeftCharacter->GetCapsuleComponent()->GetCollisionProfileName(), CapsuleCollisionShape, CollisionQueryParams);
#if !UE_BUILD_SHIPPING
	Debug_LedgeReachLoc = isBlockingHit ? reachHit.Location : ledgeReachEnd;
	Debug_LedgeReachColor = isBlockingHit ? FColor::Green : FColor::Red;
	Debug_LedgeUpMessage = !isBlockingHit ? "Can't ledge up: nothing in reach" : "";
#endif //!UE_BUILD_SHIPPING
	return isBlockingHit;
}

bool UClimbComponent::IsLedgeWithinHeightRange(FVector& outHeightDistanceTraceEnd)
{
#if !UE_BUILD_SHIPPING
	Debug_LedgeHeight = true;
#endif //!UE_BUILD_SHIPPING

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
	Debug_LedgeUpMessage = isBlockingHit ? "Can't ledge up: no ledge or obstacle obstructing" : "";
#endif //!UE_BUILD_SHIPPING
	return !isBlockingHit;
}

bool UClimbComponent::IsLedgeSurfaceWalkable(const FVector& aHeightDistanceTraceEnd, FHitResult& outSurfaceHit)
{
#if !UE_BUILD_SHIPPING
	Debug_LedgeSurface = true;
#endif //!UE_BUILD_SHIPPING

	// check that it's not a drop off and/or not walkable
	const FVector surfaceTraceEnd = aHeightDistanceTraceEnd + (FVector::DownVector * LedgeHeightMin * 2.f);

	const bool isBlockingHit = GetWorld()->LineTraceSingleByChannel(outSurfaceHit, aHeightDistanceTraceEnd, surfaceTraceEnd, ECC_WorldStatic, CollisionQueryParams);
#if !UE_BUILD_SHIPPING
	Debug_LedgeSurfaceStart = aHeightDistanceTraceEnd;
	Debug_LedgeSurfaceEnd = isBlockingHit ? outSurfaceHit.Location : surfaceTraceEnd;
	Debug_LedgeSurfaceColor = isBlockingHit ? FColor::Green : FColor::Red;
	Debug_LedgeUpMessage = !isBlockingHit ? "Can't ledge up: nothing to stand on" : "";
#endif // !UE_BUILD_SHIPPING
	return isBlockingHit;

	// TODO: check the surface normal to make sure it's physically walkable
}

bool UClimbComponent::IsEnoughRoomOnLedge(const FHitResult& aSurfaceHit, FVector& outFinalDestination)
{
#if !UE_BUILD_SHIPPING
	Debug_LedgeWidth = true;
#endif //!UE_BUILD_SHIPPING

	outFinalDestination = FVector::ZeroVector;

	// check that there is enough space on the ledge for the player's capsule component
	const FVector widthStart = aSurfaceHit.Location								// Start at the surface collision 
		+ (FVector::UpVector * CapsuleCollisionShape.GetCapsuleHalfHeight());	// raise it by half the height of the capsule since the origin is in the middle

	const FVector widthEnd = widthStart - DeftCharacter->GetActorForwardVector().GetSafeNormal(); // making the end a very small distance _closer_ to the player because of an issue with UE you can't sweep a shape literally in the exact same location
	FHitResult ledgeWidthHit;
	FCollisionQueryParams excludeSurfaceActor;
	excludeSurfaceActor.AddIgnoredActor(DeftCharacter.Get());
	excludeSurfaceActor.AddIgnoredActor(aSurfaceHit.GetActor());

	const bool isBlockingHit = GetWorld()->SweepSingleByProfile(ledgeWidthHit, widthStart, widthEnd, DeftCharacter->GetActorRotation().Quaternion(), DeftCharacter->GetCapsuleComponent()->GetCollisionProfileName(), CapsuleCollisionShape, excludeSurfaceActor);
	outFinalDestination = isBlockingHit ? ledgeWidthHit.Location : widthStart;
#if !UE_BUILD_SHIPPING
	Debug_LedgeWidthLoc = isBlockingHit ? ledgeWidthHit.Location : widthStart;
	Debug_LedgeWidthColor = isBlockingHit ? FColor::Red : FColor::Green;
	Debug_LedgeUpMessage = isBlockingHit ? "Can't ledge up: obstacles in the way on ledge" : "";
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
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::Yellow, FString::Printf(TEXT("\n-Ledge Up-\n\tLerpTime: %.2f\n\tLerpMax: %.2f\n\t%s"), LedgeUpLerpTime, LedgeUpLerpTimeMax, *Debug_LedgeUpMessage));
	
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

