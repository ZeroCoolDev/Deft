#include "GrappleComponent.h"

#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "PredictPathComponent.h"

#include "Camera/CameraComponent.h"

TAutoConsoleVariable<bool> CVar_DebugGrapple(TEXT("deft.debug.grapple"), true, TEXT(""), ECVF_Cheat);

// Sets default values for this component's properties
UGrappleComponent::UGrappleComponent()
	: GrappleAnchor(nullptr)
	, Grapple(nullptr)
	, DeftCharacter(nullptr)
	, GrappleMaxReachPoint(FVector::ZeroVector)
	, GrappleReachThreshold(0.f)
	, GrappleDistanceMax(0.f)
	, GrappleExtendSpeed(0.f)
	, GrapplePullSpeed(0.f)
	, GrappleState(GrappleStateEnum::None)
	, bIsGrappleActive(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	Grapple = CreateDefaultSubobject<USphereComponent>(TEXT("Grapple Collision Sphere"));
	Grapple->InitSphereRadius(5.f);
	Grapple->BodyInstance.SetCollisionProfileName("Grapple");

	GrappleAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("Grapple Anchor"));
}


// Called when the game starts
void UGrappleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GrappleAnchor)
		UE_LOG(LogTemp, Error, TEXT("Missing Grapple Anchor! Must be set on the character BP"));

	DeftCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (!DeftCharacter.IsValid())
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftPlayerCharacter!"));

	GrappleDistanceMax = 1000.f;
	GrappleExtendSpeed = 1100.f;
	GrapplePullSpeed = 1500.f;
	GrappleReachThreshold = 5.f;
}

void UGrappleComponent::UpdateGrappleAnchorLocation()
{
	if (UCameraComponent* cameraComponent = DeftCharacter->FindComponentByClass<UCameraComponent>())
	{
		FHitResult empty;
		FVector camLocation = cameraComponent->GetComponentLocation();
		camLocation += cameraComponent->GetRightVector() * 10.f;		// move it off to the side
		//camLocation += cameraComponent->GetForwardVector() * 20.f;	// shift it forward for viewing TODO: take out, just for debug viewing
		GrappleAnchor->K2_SetWorldLocation(camLocation, false, empty, true);
	}

	if (!bIsGrappleActive)
	{
		FHitResult empty;
		Grapple->K2_SetWorldLocation(GrappleAnchor->GetComponentLocation(), false, empty, true);
	}
}

void UGrappleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateGrappleAnchorLocation();
	ProcessGrapple(DeltaTime);

#if !UE_BUILD_SHIPPING
	DrawDebug();
#endif //!UE_BUILD_SHIPPING
}

void UGrappleComponent::DoGrapple()
{
	if (bIsGrappleActive)
		return;

	bIsGrappleActive = true;
	GrappleState = GrappleStateEnum::Extending;

	if (UCameraComponent* cameraComponent = DeftCharacter->FindComponentByClass<UCameraComponent>())
	{
		GrappleMaxReachPoint = Grapple->GetComponentLocation() + (cameraComponent->GetForwardVector() * GrappleDistanceMax);
	}
}

void UGrappleComponent::ProcessGrapple(float aDeltaTime)
{
	if (!bIsGrappleActive)
	{
		FHitResult empty;
		Grapple->K2_SetWorldLocation(GrappleAnchor->GetComponentLocation(), false, empty, true);
		return;
	}

	if (GrappleState == GrappleStateEnum::Extending)
		ExtendGrapple(aDeltaTime);
}

void UGrappleComponent::ExtendGrapple(float aDeltaTime)
{
	const FVector grappleLoc = Grapple->GetComponentLocation();
	FVector direction = GrappleMaxReachPoint - grappleLoc;

#if !UE_BUILD_SHIPPING
	Debug_GrappleDistance = direction.Length();
#endif //!UE_BUILD_SHIPPING

	if (direction.Length() < GrappleReachThreshold)
	{
		EndGrapple(false);
		return;
	}

	direction /= direction.Length();
	const FVector destination = grappleLoc + (direction * GrappleExtendSpeed * aDeltaTime);
#if !UE_BUILD_SHIPPING
	Debug_GrappleLocThisFrame = destination;
	Debug_GrappleMaxLocReached = Debug_GrappleLocThisFrame;
#endif //!UE_BUILD_SHIPPING

	// Collision check
	FCollisionQueryParams collisionParams;
	collisionParams.AddIgnoredActor(DeftCharacter.Get());
	collisionParams.AddIgnoredComponent(Grapple);
	FCollisionShape sphereShape = FCollisionShape::MakeSphere(5.f);

	FHitResult hit;
	const bool bIsBlockingHit = GetWorld()->SweepSingleByProfile(hit, grappleLoc, destination, Grapple->GetComponentRotation().Quaternion(), Grapple->GetCollisionProfileName(), sphereShape, collisionParams);
	if (bIsBlockingHit)
	{
		Debug_GrappleMaxLocReached = hit.Location;
		EndGrapple(true);
		return;
	}

	FLatentActionInfo latentInfo;
	latentInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo((USceneComponent*)Grapple, destination, Grapple->GetComponentRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);
}

void UGrappleComponent::EndGrapple(bool aApplyImpulse)
{
	bIsGrappleActive = false;

	// TODO: conditionally pull TO the player or PLAYER to the thing depending on what you hit
	if (aApplyImpulse)
	{
		CalculateAngleToReach(Grapple->GetComponentLocation());
	}
}

void UGrappleComponent::CalculateAngleToReach(const FVector& aTargetLocation)
{
	const FVector actorLoc = DeftCharacter->GetActorLocation();
	const FVector dirToGrapple = aTargetLocation - actorLoc; // vector from actor to grapple

	// the distance in the actors forward direction = the X component of the total distance
	FVector xBasis = DeftCharacter->GetActorForwardVector();
	const FVector projX = (dirToGrapple.Dot(xBasis) / xBasis.Dot(xBasis)) * xBasis;

	const float v0 = GrapplePullSpeed;							// initial velocity (i.e. speed)
	const float y0 = 0.f;										// actors starting vertical height
	const float y = FMath::Abs(aTargetLocation.Z - actorLoc.Z);	// vertical height of the grapple
	const float x = projX.Length();								// horizontal distance to the grapple
	const float g = -980.f;										//TODO: either take in, or read from WorldSettings on ctor

	// x = x0 + (v0x * t)
	// y = y0 + (v0y * t) + 1/2gt^2
	
	const float xSq = x * x;
	const float vSq = v0 * v0;
	const float gHalf = g * 0.5f;

	const float a = gHalf * (xSq / vSq);
	const float b = x;
	const float c = (y0 - y) + a;

	UE_LOG(LogTemp, Warning, TEXT("horizontal distance %.2f"), projX.Length());
	UE_LOG(LogTemp, Warning, TEXT("anchor height %2.f, relative height %.2f"), GrappleAnchor->GetComponentLocation().Z, GrappleAnchor->GetRelativeLocation().Z);

	// [ -b +- sqrt (b^2 - 4ac) ] / 2a
	const float bSsq = b * b;
	const float fourAC = 4 * a * c;
	const float twoA = 2 * a;

	UE_LOG(LogTemp, Warning, TEXT("bSsq, fourAC, twoA: %.2f, %.2f, %.2f"), bSsq, fourAC, twoA);

	if (bSsq - fourAC < 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("no solution due to negative under the radical"));
		return;
	}

	const float thetaPos = (-b + FMath::Sqrt(bSsq - fourAC)) / twoA;
	const float thetaNeg = (-b - FMath::Sqrt(bSsq - fourAC)) / twoA;

	const float rads1 = UKismetMathLibrary::Atan(thetaPos);
	const float rads2 = UKismetMathLibrary::Atan(thetaNeg);

	const float deg1 = FMath::RadiansToDegrees(rads1);
	const float deg2 = FMath::RadiansToDegrees(rads2);

#if !UE_BUILD_SHIPPING
	Debug_GrappleLaunchDeg1 = deg1;
	Debug_GrappleLaunchDeg2 = deg2;
#endif//!UE_BUILD_SHIPPING

	UE_LOG(LogTemp, Warning, TEXT("Angle %.2f or %.2f needed to reach grapple at velocity %.2f"), deg1, deg2, v0);

	UPredictPathComponent* predictPathComponent = DeftCharacter->GetPredictPathComponent();
	TArray<FVector> path;
	predictPathComponent->PredictPath_Parabola(GrapplePullSpeed, deg1, dirToGrapple, aTargetLocation, path); //TODO: obv can't use Debug_GrappleMaxLocReached need to use a non-debug variable
	UE_LOG(LogTemp, Warning, TEXT("Predicted Path contains %d points"), path.Num());
}

#if !UE_BUILD_SHIPPING
void UGrappleComponent::DrawDebug()
{
	if (!CVar_DebugGrapple.GetValueOnGameThread())
		return;

	GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsGrappleActive ? FColor::Green : FColor::White, FString::Printf(TEXT("Distance till max: %.2f"), Debug_GrappleDistance));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, TEXT("-Grapple-"));


	//DrawDebugLine(GetWorld(), GrappleAnchor->GetComponentLocation(), GrappleMaxReachPoint, FColor::Yellow);

	DrawDebugSphere(GetWorld(), Grapple->GetComponentLocation(), Grapple->GetUnscaledSphereRadius(), 12, FColor::Blue);
	DrawDebugSphere(GetWorld(), GrappleAnchor->GetComponentLocation(), 10.f, 12, FColor::White);
	DrawDebugSphere(GetWorld(), GrappleMaxReachPoint, 10.f, 12, FColor::Yellow);

	if (!bIsGrappleActive)
	{
		DrawDebugSphere(GetWorld(), Debug_GrappleMaxLocReached, Grapple->GetUnscaledSphereRadius(), 12, FColor::Red);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsGrappleActive ? FColor::Green : FColor::White, FString::Printf(TEXT("Attachment Loc: (%.2f, %.2f, %.2f)"), Debug_GrappleMaxLocReached.X, Debug_GrappleMaxLocReached.Y, Debug_GrappleMaxLocReached.Z));
		//DrawDebugSphere(GetWorld(), Debug_GrappleLocThisFrame, 5.f, 8, FColor::Purple, false, 0.5f);
	}

	if (UCameraComponent* cameraComponent = DeftCharacter->FindComponentByClass<UCameraComponent>())
	{
		//DrawDebugLine(GetWorld(), DeftCharacter->GetActorLocation(), DeftCharacter->GetActorLocation() + (cameraComponent->GetForwardVector() * 100.f), FColor::Cyan);
	}
}
#endif //!UE_BUILD_SHIPPING

