#include "GrappleComponent.h"

#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"
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

	GrappleDistanceMax = 500.f;
	GrappleSpeed = 700.f;
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

	const FVector actorForward = DeftCharacter->GetActorForwardVector();
	GrappleMaxReachPoint = Grapple->GetComponentLocation() + (actorForward * GrappleDistanceMax);
}

void UGrappleComponent::ProcessGrapple(float aDeltaTime)
{
	if (!bIsGrappleActive)
	{
		FHitResult empty;
		Grapple->K2_SetWorldLocation(GrappleAnchor->GetComponentLocation(), false, empty, true);
		return;
	}

	const FVector grappleLoc = Grapple->GetComponentLocation();
	FVector direction = GrappleMaxReachPoint - grappleLoc;

#if !UE_BUILD_SHIPPING
	Debug_GrappleDistance = direction.Length();
#endif //!UE_BUILD_SHIPPING

	if (direction.Length() < GrappleReachThreshold)
	{
		EndGrapple();
		return;
	}

	direction /= direction.Length();
	const FVector destination = grappleLoc + (direction * GrappleSpeed * aDeltaTime);
#if !UE_BUILD_SHIPPING
	Debug_GrappleLocThisFrame = destination;
#endif //!UE_BUILD_SHIPPING

	// Collision check


	FLatentActionInfo latentInfo;
	latentInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo((USceneComponent*)Grapple, destination, Grapple->GetComponentRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);
}

void UGrappleComponent::EndGrapple()
{
	bIsGrappleActive = false;
}

#if !UE_BUILD_SHIPPING
void UGrappleComponent::DrawDebug()
{
	if (!CVar_DebugGrapple.GetValueOnGameThread())
		return;

	GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsGrappleActive ? FColor::Green : FColor::White, FString::Printf(TEXT("Distance till max: %.2f"), Debug_GrappleDistance));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, TEXT("-Grapple-"));


	DrawDebugLine(GetWorld(), GrappleAnchor->GetComponentLocation(), GrappleMaxReachPoint, FColor::Yellow);

	DrawDebugSphere(GetWorld(), Grapple->GetComponentLocation(), 5.f, 12, FColor::Blue);
	DrawDebugSphere(GetWorld(), GrappleAnchor->GetComponentLocation(), 10.f, 12, FColor::White);
	DrawDebugSphere(GetWorld(), GrappleMaxReachPoint, 10.f, 12, FColor::Yellow);

	if (bIsGrappleActive)
	{
		DrawDebugSphere(GetWorld(), Debug_GrappleLocThisFrame, 5.f, 8, FColor::Purple, false, 0.5f);
	}
}
#endif //!UE_BUILD_SHIPPING

