#include "CameraMovementComponent.h"

#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values for this component's properties
UCameraMovementComponent::UCameraMovementComponent()
	: WalkBobbleCurve(nullptr)
	, DeftPlayerCharacter(nullptr)
	, BobbleTarget(nullptr)
	, PreviousInputVector(FVector2D::ZeroVector)
	, WalkBobbleTime(0.f)
	, WalkBobbleMaxTime(0.f)
	, PrevWalkBobbleVal(0.f)
	, RollLerpTimeMax(0.f)
	, RollLerpTime(0.f)
	, RollLerpStart(0.f)
	, RollLerpEnd(0.f)
	, HighestRollTimeAchieved(0.f)
	, UnrollLerpTime(0.f)
	, bUnrollFromLeft(false)
	, bNeedsUnroll(false)
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

	// Bobble Setup
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

	// Roll Setup
	RollLerpTimeMax = 0.3f;
	RollLerpStart = 0.f;
	RollLerpEnd = 3.f;

	// Land Dip setup
	UDeftCharacterMovementComponent* deftCharacterMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftPlayerCharacter->GetMovementComponent());
	if (!deftCharacterMovementComponent)
		UE_LOG(LogTemp, Error, TEXT("Failed to get DeftCharacterMovementComponent"));

	deftCharacterMovementComponent->OnLandedFromAir.BindUObject(this, &UCameraMovementComponent::PerformCameraLandDip);
}

void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessCameraBobble(DeltaTime);
	ProcessCameraRoll(DeltaTime);
}

void UCameraMovementComponent::ProcessCameraBobble(float aDeltaTime)
{
	if (!DeftPlayerCharacter->GetCharacterMovement()->IsMovingOnGround())
		return;

	if (DeftPlayerCharacter->GetVelocity() == FVector::ZeroVector)
		return;

	if (!BobbleTarget.IsValid())
		return;

	if (WalkBobbleTime >= WalkBobbleMaxTime)
		WalkBobbleTime = 0.f;

	WalkBobbleTime += aDeltaTime;
	if (WalkBobbleTime <= WalkBobbleMaxTime)
	{
		const float bobbleCurveVal = WalkBobbleCurve->GetFloatValue(WalkBobbleTime);
		const float bobbleCurveValDelta = bobbleCurveVal - PrevWalkBobbleVal;

		PrevWalkBobbleVal = bobbleCurveVal;

		const FVector cameraLocation = BobbleTarget->GetRelativeLocation();
		const FVector newLocation = cameraLocation - FVector(0.f, 0.f, bobbleCurveValDelta);

		BobbleTarget->SetRelativeLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}

}

void UCameraMovementComponent::ProcessCameraRoll(float aDeltaTime)
{
	// Camera roll
	const FVector2D& inputVector = DeftPlayerCharacter->GetInputMoveVector();

	const bool isLeaningRight = inputVector.X > 0;
	const bool wasLeaningRight = PreviousInputVector.X > 0;

	const bool isLeaningLeft = inputVector.X < 0;
	const bool wasLeaningLeft = PreviousInputVector.X < 0;

	const bool startedLeaningRight = isLeaningRight && !wasLeaningRight;
	const bool stoppedLeaningRight = wasLeaningRight && !isLeaningRight;
	
	const bool startedLeaningLeft = isLeaningLeft && !wasLeaningLeft;
	const bool stoppedLeaningLeft = wasLeaningLeft && !isLeaningLeft;

	// Will only trigger for the frame that input stopped
	if (stoppedLeaningRight || stoppedLeaningLeft)
	{
		bUnrollFromLeft = wasLeaningLeft;
		RollLerpTime = 0.f; // reset roll lerp since we're unrolling so that the time isn't carried over to an different lean

		// Time to unroll should be conversely proportional to how much time was rolled
		UnrollLerpTime = RollLerpTimeMax - HighestRollTimeAchieved;
		bNeedsUnroll = true;
		HighestRollTimeAchieved = 0.f;
	}

	// Rolling back into origin
	if (bNeedsUnroll)
	{
		UnrollLerpTime += aDeltaTime;
		if (UnrollLerpTime > RollLerpTimeMax)
		{
			UnrollLerpTime = RollLerpTimeMax;
			bNeedsUnroll = false; // this is the last unroll frame we need
		}

		// lerp!
		const float percent = UnrollLerpTime / RollLerpTimeMax;
		float unroll = FMath::Lerp(RollLerpEnd, RollLerpStart, percent);
		if (bUnrollFromLeft)
			unroll *= -1.f;

		const FRotator rotation = DeftPlayerCharacter->GetController()->GetControlRotation();
		const FRotator newRotation(rotation.Pitch, rotation.Yaw, unroll);
		DeftPlayerCharacter->GetController()->SetControlRotation(newRotation);
	}
	else if (isLeaningRight || isLeaningLeft)	// Rolling out can only occur if we're not unrolling
	{
		if (startedLeaningRight || startedLeaningLeft)
			RollLerpTime = 0.f;

		RollLerpTime += aDeltaTime;
		if (RollLerpTime > RollLerpTimeMax)
			RollLerpTime = RollLerpTimeMax;
		HighestRollTimeAchieved = RollLerpTime;

		// lerp!
		const float percent = RollLerpTime / RollLerpTimeMax;
		float roll = FMath::Lerp(RollLerpStart, RollLerpEnd, percent);
		if (isLeaningLeft)
			roll *= -1.f;
	
		const FRotator rotation = DeftPlayerCharacter->GetController()->GetControlRotation();
		const FRotator newRotation(rotation.Pitch, rotation.Yaw, roll);
		DeftPlayerCharacter->GetController()->SetControlRotation(newRotation);
	}

	PreviousInputVector = inputVector;
}

void UCameraMovementComponent::PerformCameraLandDip()
{
	UE_LOG(LogTemp, Log, TEXT("Landed!"));
}

