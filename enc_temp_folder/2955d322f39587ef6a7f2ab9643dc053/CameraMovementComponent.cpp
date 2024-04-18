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
	, CameraTarget(nullptr)
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
	, UnrollLerpTimeMax(0.f)
	, DipLerpTimeMax(0.f)
	, DipLerpTime(0.f)
	, PrevDipVal(0.f)
	, BackTiltLerpTime(0.f)
	, BackTiltLerpTimeMax(0.f)
	, BackTiltStart(0.f)
	, BackTiltEnd(0.f)
	, HighestTiltTimeAchieved(0.f)
	, UnBackTiltLerpTime(0.f)
	, UnBackTiltLerpTimeMax(0.f)
	, bIsBackTiltActive(false)
	, bIsUnBackTiltActive(false)
	, bNeedsUnroll(false)
	, bUnrollFromLeft(false)
	, bNeedsDip(false)
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

	CameraTarget = DeftPlayerCharacter->FindComponentByClass<USpringArmComponent>();
	if (!CameraTarget.IsValid())
		UE_LOG(LogTemp, Error, TEXT("Failed to set CameraTarget!"));

	// Roll Setup
	RollLerpTimeMax = 0.3f;
	RollLerpStart = 0.f;
	RollLerpEnd = 3.f;
	UnrollLerpTimeMax = 0.1f;

	// Land Dip setup
	if (LandedFromAirDipCuve)
	{
		UDeftCharacterMovementComponent* deftCharacterMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftPlayerCharacter->GetMovementComponent());
		if (!deftCharacterMovementComponent)
			UE_LOG(LogTemp, Error, TEXT("Failed to get DeftCharacterMovementComponent"));

		deftCharacterMovementComponent->OnLandedFromAir.AddUObject(this, &UCameraMovementComponent::OnLandedFromAir);

		float unusedMin;
		LandedFromAirDipCuve->GetTimeRange(unusedMin, DipLerpTimeMax);
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Landed From Air Dip curve is invalid"));

	BackTiltLerpTimeMax = 0.3f;
	BackTiltStart = 0.f;
	BackTiltEnd = 3.f;
	UnBackTiltLerpTimeMax = 0.1f;
}

void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessCameraBobble(DeltaTime);
	ProcessCameraRoll(DeltaTime);
	ProcessCameraDip(DeltaTime);
	ProcessCameraTilt(DeltaTime);

	PreviousInputVector = DeftPlayerCharacter->GetInputMoveVector();
}

void UCameraMovementComponent::ProcessCameraBobble(float aDeltaTime)
{
	if (bNeedsDip) // don't play dip and bobble at the same time
		return;

	if (!WalkBobbleCurve)
		return;

	if (!DeftPlayerCharacter->GetCharacterMovement()->IsMovingOnGround())
		return;

	if (DeftPlayerCharacter->GetVelocity() == FVector::ZeroVector)
		return;

	if (!CameraTarget.IsValid())
		return;

	if (WalkBobbleTime >= WalkBobbleMaxTime)
		WalkBobbleTime = 0.f;

	WalkBobbleTime += aDeltaTime;
	if (WalkBobbleTime <= WalkBobbleMaxTime)
	{
		const float bobbleCurveVal = WalkBobbleCurve->GetFloatValue(WalkBobbleTime);
		const float bobbleCurveValDelta = bobbleCurveVal - PrevWalkBobbleVal;

		PrevWalkBobbleVal = bobbleCurveVal;

		const FVector cameraLocation = CameraTarget->GetRelativeLocation();
		const FVector newLocation = cameraLocation - FVector(0.f, 0.f, bobbleCurveValDelta);

		CameraTarget->SetRelativeLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);
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
		// Time to unroll is faster than rolling so need to map time from roll to unroll range
		const float unrollLerpInRollRange = RollLerpTimeMax - HighestRollTimeAchieved;
		UnrollLerpTime = (unrollLerpInRollRange / RollLerpTimeMax) * UnrollLerpTimeMax;

		bUnrollFromLeft = wasLeaningLeft;
		RollLerpTime = 0.f; // reset roll lerp since we're unrolling so that the time isn't carried over to an different lean
		bNeedsUnroll = true;
		HighestRollTimeAchieved = 0.f;
	}

	// TODO: Unrolling in should be quicker than rolling out
	// Rolling back into origin
	if (bNeedsUnroll)
	{
		UnrollLerpTime += aDeltaTime;
		if (UnrollLerpTime > UnrollLerpTimeMax)
		{
			UnrollLerpTime = UnrollLerpTimeMax;
			bNeedsUnroll = false; // this is the last unroll frame we need
		}

		// lerp!
		const float percent = UnrollLerpTime / UnrollLerpTimeMax;
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
}

void UCameraMovementComponent::ProcessCameraDip(float aDeltaTime)
{
	if (!bNeedsDip)
		return;

	if (!CameraTarget.IsValid())
		return;

	if (DipLerpTime >= DipLerpTimeMax)
	{
		bNeedsDip = false;
		return;
	}

	DipLerpTime += aDeltaTime;
	if (DipLerpTime <= DipLerpTimeMax)
	{
		const float dipCurveVal = LandedFromAirDipCuve->GetFloatValue(DipLerpTime);
		const float dipCurveValDelta = dipCurveVal - PrevDipVal;

		PrevDipVal = dipCurveVal;

		const FVector cameraLocation = CameraTarget->GetRelativeLocation();
		const FVector newLocation = cameraLocation - FVector(0.f, 0.f, dipCurveValDelta);

		CameraTarget->SetRelativeLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void UCameraMovementComponent::ProcessCameraTilt(float aDeltaTime)
{
	const bool isBackwardsInput = DeftPlayerCharacter->GetInputMoveVector().Y < 0.f;
	const bool wasBackwardsInput = PreviousInputVector.Y < 0.f;
	//UE_LOG(LogTemp, Warning, TEXT("Y %.2f, Prev Y %.2f"), DeftPlayerCharacter->GetInputMoveVector().Y, PreviousInputVector.Y);

	// TODO: So there is an issue here that tilting manually conflicts with the mouse pitch...which makes sense
	// just not sure how to resolve it atm
	// I think it might just have to be additive with the mouse pitch so like sending the input up to the playerCharacter::Look?
	// AddControllerPitchInput

	if (!isBackwardsInput && wasBackwardsInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("untilting"));
		// un-tilt
		bIsUnBackTiltActive = true;
		// Time to un-tilt is faster than tilting so need to map time from tilt to un-tilt range
		const float untiltLerpInTiltRange = BackTiltLerpTimeMax - HighestTiltTimeAchieved;
		UnBackTiltLerpTime = (untiltLerpInTiltRange / BackTiltLerpTimeMax) * UnBackTiltLerpTimeMax;

		bIsBackTiltActive = false;
		BackTiltLerpTime = 0.f;
	}
	else if (isBackwardsInput && !wasBackwardsInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("tilt"));
		// tilt
		bIsBackTiltActive = true;
		BackTiltLerpTime = 0.f;

		bIsUnBackTiltActive = false;
		UnBackTiltLerpTime = 0.f;
	}

	if (bIsBackTiltActive)
	{
		BackTiltLerpTime += aDeltaTime;
		// reached the end of the lerp
		if (BackTiltLerpTime > BackTiltLerpTimeMax)
			BackTiltLerpTime = BackTiltLerpTimeMax;
		HighestTiltTimeAchieved = BackTiltLerpTime;

		//UE_LOG(LogTemp, Warning, TEXT("BackTiltLerpTime: %.2f"), BackTiltLerpTime);

		// lerp!
		const float percent = BackTiltLerpTime / BackTiltLerpTimeMax;
		float backTilt = FMath::Lerp(BackTiltStart, BackTiltEnd, percent);

		const FRotator rotation = DeftPlayerCharacter->GetController()->GetControlRotation();
		const FRotator newRotation(backTilt, rotation.Yaw, rotation.Roll);
		DeftPlayerCharacter->GetController()->SetControlRotation(newRotation);
	}
	else if (bIsUnBackTiltActive)
	{
		//UE_LOG(LogTemp, Warning, TEXT("UnBackTiltLerpTime: %.2f"), UnBackTiltLerpTime);

		UnBackTiltLerpTime += aDeltaTime;
		if (UnBackTiltLerpTime > UnBackTiltLerpTimeMax)
		{
			UE_LOG(LogTemp, Warning, TEXT("Unbacktilt complete"));
			UnBackTiltLerpTime = UnBackTiltLerpTimeMax;
			bIsUnBackTiltActive = false; // this is the last untilt frame
		}

		// lerp!
		const float percent = UnBackTiltLerpTime / UnBackTiltLerpTimeMax;
		float unbacktilt = FMath::Lerp(BackTiltEnd, BackTiltStart, percent);

		const FRotator rotation = DeftPlayerCharacter->GetController()->GetControlRotation();
		const FRotator newRotation(unbacktilt, rotation.Yaw, rotation.Roll);
		DeftPlayerCharacter->GetController()->SetControlRotation(newRotation);
	}
}

void UCameraMovementComponent::OnLandedFromAir()
{
	bNeedsDip = true;
	DipLerpTime = 0.f;
}

