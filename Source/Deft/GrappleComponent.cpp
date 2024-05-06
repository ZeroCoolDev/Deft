#include "GrappleComponent.h"

#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetSystemLibrary.h"

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
	, GrappleSpeed(0.f)
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
	GrappleSpeed = 1100.f;
	GrappleReachThreshold = 5.f;
}

void UGrappleComponent::UpdateGrappleAnchorLocation()
{
	if (UCameraComponent* cameraComponent = DeftCharacter->FindComponentByClass<UCameraComponent>())
	{
		FHitResult empty;
		FVector camLocation = cameraComponent->GetComponentLocation();
		camLocation += cameraComponent->GetRightVector() * 10.f;	// move it off to the side
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
	const FVector destination = grappleLoc + (direction * GrappleSpeed * aDeltaTime);
#if !UE_BUILD_SHIPPING
	Debug_GrappleLocThisFrame = destination;
	Debug_GrappleMaxLocReached = Debug_GrappleLocThisFrame;
#endif //!UE_BUILD_SHIPPING

	// Collision check
	FCollisionQueryParams collisionParams;
	collisionParams.AddIgnoredActor(DeftCharacter.Get());
	collisionParams.AddIgnoredComponent(Grapple);
	FCollisionShape sphereShape = FCollisionShape::MakeSphere(Grapple->GetUnscaledSphereRadius() * 2.f);

	FHitResult hit;
	const bool bIsBlockingHit = GetWorld()->SweepSingleByProfile(hit, grappleLoc, grappleLoc + (DeftCharacter->GetActorForwardVector() * 100.f), Grapple->GetComponentRotation().Quaternion(), Grapple->GetCollisionProfileName(), sphereShape, collisionParams);
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
		if (UDeftCharacterMovementComponent* deftMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftCharacter->GetMovementComponent()))
		{
			const FVector grappleDir = Grapple->GetComponentLocation() - DeftCharacter->GetActorLocation();
			const FVector impulseDir = grappleDir + (FVector::UpVector * 100.f);
			// TODO: we gotta rotate it _up_ by some angle, but I don't know what angle that is just yet
			// TODO: The speed
			deftMovementComponent->DoImpulse(impulseDir * 2.f);
		}
}

#if !UE_BUILD_SHIPPING
void UGrappleComponent::DrawDebug()
{
	if (!CVar_DebugGrapple.GetValueOnGameThread())
		return;

	GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsGrappleActive ? FColor::Green : FColor::White, FString::Printf(TEXT("Distance till max: %.2f"), Debug_GrappleDistance));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, TEXT("-Grapple-"));


	DrawDebugLine(GetWorld(), GrappleAnchor->GetComponentLocation(), GrappleMaxReachPoint, FColor::Yellow);

	DrawDebugSphere(GetWorld(), Grapple->GetComponentLocation(), Grapple->GetUnscaledSphereRadius(), 12, FColor::Blue);
	DrawDebugSphere(GetWorld(), GrappleAnchor->GetComponentLocation(), 10.f, 12, FColor::White);
	DrawDebugSphere(GetWorld(), GrappleMaxReachPoint, 10.f, 12, FColor::Yellow);

	if (!bIsGrappleActive)
	{
		DrawDebugSphere(GetWorld(), Debug_GrappleMaxLocReached, Grapple->GetUnscaledSphereRadius(), 12, FColor::Red);
		//DrawDebugSphere(GetWorld(), Debug_GrappleLocThisFrame, 5.f, 8, FColor::Purple, false, 0.5f);
	}

	if (UCameraComponent* cameraComponent = DeftCharacter->FindComponentByClass<UCameraComponent>())
	{
		//DrawDebugLine(GetWorld(), DeftCharacter->GetActorLocation(), DeftCharacter->GetActorLocation() + (cameraComponent->GetForwardVector() * 100.f), FColor::Cyan);
	}
}
#endif //!UE_BUILD_SHIPPING

