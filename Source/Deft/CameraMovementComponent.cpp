#include "CameraMovementComponent.h"

#include "DeftPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UCameraMovementComponent::UCameraMovementComponent()
	: WalkBobbleCurve(nullptr)
	, DeftPlayerCharacter(nullptr)
	, BobbleTarget(nullptr)
	, WalkBobbleTime(0.f)
	, WalkBobbleMaxTime(0.f)
	, PrevWalkBobbleVal(0.f)
	, bIsWalkBobbleActive(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UCameraMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

	DeftPlayerCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (!DeftPlayerCharacter.IsValid())
		UE_LOG(LogTemp, Error, TEXT("Missing owner"));

	if (WalkBobbleCurve)
	{
		float unusedMin;
		WalkBobbleCurve->GetTimeRange(unusedMin, WalkBobbleMaxTime);
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Walk Bobble curve is invalid"));

	BobbleTarget = DeftPlayerCharacter->FindComponentByClass<USpringArmComponent>();
	if (!BobbleTarget.IsValid())
		UE_LOG(LogTemp, Error, TEXT("Failed to set BobbleTarget!"));
}


// Called every frame
bool temp = false;
void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!DeftPlayerCharacter->GetCharacterMovement()->IsMovingOnGround())
	{
		UE_LOG(LogTemp, Warning, TEXT("not running bobble because we're not walking"));
		return;
	}

	if (DeftPlayerCharacter->GetVelocity() == FVector::ZeroVector)
		return;

	if (!BobbleTarget.IsValid())
		return;

	if (WalkBobbleTime >= WalkBobbleMaxTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reached end of bobble curve but still moving so resetting to beginning"));
		WalkBobbleTime = 0.f;
	}

	WalkBobbleTime += DeltaTime;
	if (WalkBobbleTime <= WalkBobbleMaxTime)
	{
		const float bobbleCurveVal = WalkBobbleCurve->GetFloatValue(WalkBobbleTime);
		const float bobbleCurveValDelta = bobbleCurveVal - PrevWalkBobbleVal;
		UE_LOG(LogTemp, Log, TEXT("running bobble!"));

		PrevWalkBobbleVal = bobbleCurveVal;

		const FVector cameraLocation = BobbleTarget->GetRelativeLocation();
		const FVector newLocation = cameraLocation - FVector(0.f, 0.f, bobbleCurveValDelta);

		BobbleTarget->SetRelativeLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

