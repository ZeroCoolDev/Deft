#include "CameraMovementComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UCameraMovementComponent::UCameraMovementComponent()
	: WalkBobbleCurve(nullptr)
	, DeftPlayerCharacter(nullptr)
	, MoveableCamera(nullptr)
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
}


// Called every frame
void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeftPlayerCharacter->GetCharacterMovement()->GetGroundMovementMode() != MOVE_Walking)
		return;

	if (DeftPlayerCharacter->GetVelocity() == FVector::ZeroVector)
		return;

	if (!MoveableCamera.IsValid())
		return;

	if (WalkBobbleTime >= WalkBobbleMaxTime)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reached end of bobble curve but still moving so resetting to beginning"));
		WalkBobbleTime = 0.f;
	}

	WalkBobbleTime += DeltaTime;
	if (WalkBobbleTime <= WalkBobbleMaxTime)
	{
		UE_LOG(LogTemp, Log, TEXT("running bobble!"));
		const float bobbleCurveVal = WalkBobbleCurve->GetFloatValue(WalkBobbleTime);
		const float bobbleCurveValDelta = bobbleCurveVal - PrevWalkBobbleVal;

		PrevWalkBobbleVal = bobbleCurveVal;

		const FVector cameraLocation = MoveableCamera->GetComponentTransform().GetLocation();
		const FVector newLocation = cameraLocation - FVector(0.f, 0.f, bobbleCurveValDelta);

		FLatentActionInfo latendInfo;
		latendInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo((USceneComponent*)MoveableCamera.Get(), newLocation, MoveableCamera->GetComponentRotation(), false, false, 0.f, true, EMoveComponentAction::Type::Move, latendInfo);
	}
}

