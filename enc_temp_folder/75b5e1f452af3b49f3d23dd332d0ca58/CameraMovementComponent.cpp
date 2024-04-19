#include "CameraMovementComponent.h"

#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

//TODO: maybesplit this into 4 CameraMovementComponent classes: Bobble, Roll, Dip(Land), Pitch 

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
	, PitchLerpTime(0.f)
	, PitchLerpTimeMax(0.f)
	, PitchStart(0.f)
	, PitchEnd(0.f)
	, PrevPitch(0.f)
	, HighestTiltTimeAchieved(0.f)
	, UnPitchLerpTime(0.f)
	, UnPitchLerpTimeMax(0.f)
	, PrevUnPitch(0.f)
	, SlidePoseLerpTime(0.f)
	, SlidePoseLerpTimeMax(0.f)
	, SlidePoseStart(0.f)
	, SlidePoseEnd(0.f)
	, PrevSlidePose(0.f)
	, SlidePoseRollEndOverride(0.f)
	, SlidePoseRollLerpTimeMaxOverride(0.f)
	, bIsPitchActive(false)
	, bIsUnPitchActive(false)
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
		float unusedMin;
		LandedFromAirDipCuve->GetTimeRange(unusedMin, DipLerpTimeMax);
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Landed From Air Dip curve is invalid"));

	// Pitch setup
	PitchLerpTimeMax = 0.3f;
	PitchStart = 0.f;
	PitchEnd = 2.f;
	UnPitchLerpTimeMax = 0.15f;

	// listeners setup
	UDeftCharacterMovementComponent* deftCharacterMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftPlayerCharacter->GetMovementComponent());
	if (!deftCharacterMovementComponent)
		UE_LOG(LogTemp, Error, TEXT("Failed to get DeftCharacterMovementComponent"));

	deftCharacterMovementComponent->OnLandedFromAir.AddUObject(this, &UCameraMovementComponent::OnLandedFromAir);
	deftCharacterMovementComponent->OnSlideActionOccured.AddUObject(this, &UCameraMovementComponent::OnSlideActionOccured);

	// Slide Pose setup
	SlidePoseLerpTimeMax = 0.15f;
	SlidePoseRollEndOverride = 20.f;
	SlidePoseRollLerpTimeMaxOverride = 0.3f;
	if (ACharacter* characterOwner = Cast<ACharacter>(GetOwner()))
	{
		SlidePoseStart = characterOwner->GetCapsuleComponent()->GetRelativeLocation().Z;//130
		SlidePoseEnd = 42.f;
	}
}

void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ProcessCameraBobble(DeltaTime);
	ProcessCameraRoll(DeltaTime);
	ProcessCameraDip(DeltaTime);
	ProcessCameraPitch(DeltaTime);
	ProcessCameraSlidePose(DeltaTime);

	PreviousInputVector = DeftPlayerCharacter->GetInputMoveVector();
}

void UCameraMovementComponent::ProcessCameraBobble(float aDeltaTime)
{
	if (bNeedsDip) // don't play dip and bobble at the same time otherwise they fight eachtoher
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
		PreUnRoll(wasLeaningLeft);

	// Rolling back into origin
	if (bNeedsUnroll)
	{
		UnRoll(aDeltaTime);
	}
	else if (isLeaningRight || isLeaningLeft)
	{
		if (startedLeaningRight || startedLeaningLeft)
			PreRoll();

		Roll(aDeltaTime, isLeaningLeft);
	}
}

void UCameraMovementComponent::PreRoll()
{
	RollLerpTime = 0.f;
}

void UCameraMovementComponent::Roll(float aDeltaTime, bool aIsLeaningLeft)
{
	RollLerpTime += aDeltaTime;
	if (RollLerpTime > RollLerpTimeMax)
		RollLerpTime = RollLerpTimeMax;
	HighestRollTimeAchieved = RollLerpTime;

	// lerp!
	const float percent = RollLerpTime / RollLerpTimeMax;
	float roll = FMath::Lerp(RollLerpStart, bIsSlidePoseActive ? SlidePoseRollEndOverride : RollLerpEnd, percent);
	if (aIsLeaningLeft)
		roll *= -1.f;

	const FRotator rotation = DeftPlayerCharacter->GetController()->GetControlRotation();
	const FRotator newRotation(rotation.Pitch, rotation.Yaw, roll);
	DeftPlayerCharacter->GetController()->SetControlRotation(newRotation);
}

void UCameraMovementComponent::PreUnRoll(bool aWasLeaningLeft)
{
	// Time to unroll is faster than rolling so need to map time from roll to unroll range
	const float unrollLerpInRollRange = RollLerpTimeMax - HighestRollTimeAchieved;
	UnrollLerpTime = (unrollLerpInRollRange / RollLerpTimeMax) * (bIsUnSlidePoseActive ? SlidePoseRollLerpTimeMaxOverride : UnrollLerpTimeMax);

	bUnrollFromLeft = aWasLeaningLeft;
	RollLerpTime = 0.f; // reset roll lerp since we're unrolling so that the time isn't carried over to an different lean
	bNeedsUnroll = true;
	HighestRollTimeAchieved = 0.f;
}

void UCameraMovementComponent::UnRoll(float aDeltaTime)
{
	UnrollLerpTime += aDeltaTime;
	const float unrollLerpTime = bIsUnSlidePoseActive ? SlidePoseRollLerpTimeMaxOverride : UnrollLerpTimeMax;
	if (UnrollLerpTime > unrollLerpTime)
	{
		UnrollLerpTime = unrollLerpTime;
		bNeedsUnroll = false; // this is the last unroll frame we need
	}

	// lerp!
	const float percent = UnrollLerpTime / unrollLerpTime;
	float unroll = FMath::Lerp(bIsUnSlidePoseActive ? SlidePoseRollEndOverride : RollLerpEnd, RollLerpStart, percent);
	if (bUnrollFromLeft)
		unroll *= -1.f;

	const FRotator rotation = DeftPlayerCharacter->GetController()->GetControlRotation();
	const FRotator newRotation(rotation.Pitch, rotation.Yaw, unroll);
	DeftPlayerCharacter->GetController()->SetControlRotation(newRotation);
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

void UCameraMovementComponent::ProcessCameraPitch(float aDeltaTime)
{
	const bool isBackwardsInput = DeftPlayerCharacter->GetInputMoveVector().Y < 0.f;
	const bool wasBackwardsInput = PreviousInputVector.Y < 0.f;

	if (!isBackwardsInput && wasBackwardsInput)
		PreUnPitch();
	else if (isBackwardsInput && !wasBackwardsInput)
		PrePitch();

	if (bIsPitchActive)
	{
		PitchLerpTime += aDeltaTime;
		// once we reach the end of the lerp cease adding pitch
		if (PitchLerpTime > PitchLerpTimeMax)
			return;
		HighestTiltTimeAchieved = PitchLerpTime;

		// lerp!
		const float percent = PitchLerpTime / PitchLerpTimeMax;
		const float pitch = FMath::Lerp(PitchStart, PitchEnd, percent);
		const float pitchDelta = FMath::Abs(PrevPitch - pitch);

		DeftPlayerCharacter->AddControllerPitchInput(-pitchDelta);

		PrevPitch = pitch;
	}
	else if (bIsUnPitchActive)
	{
		UnPitchLerpTime += aDeltaTime;
		if (UnPitchLerpTime > UnPitchLerpTimeMax)
		{
			UnPitchLerpTime = UnPitchLerpTimeMax;
			bIsUnPitchActive = false; // this is the last untilt frame
		}

		// lerp!
		const float percent = UnPitchLerpTime / UnPitchLerpTimeMax;
		float unPitch = FMath::Lerp(PitchEnd, PitchStart, percent);
		const float unPitchDetlta = FMath::Abs(PrevUnPitch - unPitch);
		DeftPlayerCharacter->AddControllerPitchInput(unPitchDetlta);

		PrevUnPitch = unPitch;
	}
}

void UCameraMovementComponent::PrePitch()
{
	bIsPitchActive = true;
	PitchLerpTime = 0.f;
	PrevPitch = 0.f;

	// reset un-tilt
	bIsUnPitchActive = false;
	UnPitchLerpTime = 0.f;
}

void UCameraMovementComponent::PreUnPitch()
{
	bIsUnPitchActive = true;

	// Time to un-tilt is faster than tilting so need to map time from tilt to un-tilt range
	// 100% highest tilt should start at 0% untilt since the range is opposite
	// ex: tilt 0->3 while untilt 3->0
	const float untiltLerpInTiltRange = PitchLerpTimeMax - HighestTiltTimeAchieved;
	UnPitchLerpTime = (untiltLerpInTiltRange / PitchLerpTimeMax) * UnPitchLerpTimeMax;

	// store the previous pitch as the starting point for smooth transition back from tilting otherwise there is a large delta since they don't have the same lerp time range
	const float percent = UnPitchLerpTime / UnPitchLerpTimeMax;
	const float unPitch = FMath::Lerp(PitchEnd, PitchStart, percent);
	PrevUnPitch = unPitch;

	// reset tilt
	bIsPitchActive = false;
	PitchLerpTime = 0.f;
}

void UCameraMovementComponent::ProcessCameraSlidePose(float aDeltaTime)
{
	// quickly lerp camera either down or up based off whether we're sliding or not
	if (!bIsSlidePoseActive && !bIsUnSlidePoseActive)
		return;

	if (!CameraTarget.IsValid())
		return;

	if (bIsSlidePoseActive || bIsUnSlidePoseActive)
	{
		SlidePoseLerpTime += aDeltaTime;
		if (SlidePoseLerpTime > SlidePoseLerpTimeMax)
		{
			if (bIsSlidePoseActive)
				return;

			if (bIsUnSlidePoseActive)
			{
				// restored original pose 
				SlidePoseLerpTime = 0.f;
				bIsSlidePoseActive = false;
				bIsUnSlidePoseActive = false;
				return;
			}
		}

		// TODO: This is a little messy, cleanup rather than having to do a buncha terinaries

		// lerp!
		const float percent = SlidePoseLerpTime / SlidePoseLerpTimeMax;
		// lerp either moves camera from higher to lower (sliding) or lower to higher (done sliding)
		const float newCameraZPos = bIsSlidePoseActive ? FMath::Lerp(SlidePoseStart, SlidePoseEnd, percent) : FMath::Lerp(SlidePoseEnd, SlidePoseStart, percent);
		const float cameraZPosDelta = FMath::Abs(newCameraZPos - PrevSlidePose);
		UE_LOG(LogTemp, Warning, TEXT("Z pos %.2f, delta %.2f"), newCameraZPos, cameraZPosDelta);

		const FVector cameraLocation = CameraTarget->GetRelativeLocation();
		const FVector destinationLocation = cameraLocation - FVector(0.f, 0.f, bIsSlidePoseActive ? cameraZPosDelta : -cameraZPosDelta);
		CameraTarget->SetRelativeLocation(destinationLocation);

		if (bIsSlidePoseActive)
		{
			constexpr bool isLeaningLeft = true;
			Roll(aDeltaTime, isLeaningLeft);
		}
		else
			UnRoll(aDeltaTime);

		PrevSlidePose = newCameraZPos;
	}
}

void UCameraMovementComponent::EnterSlidePose()
{
	SlidePoseLerpTime = 0.f;
	PrevSlidePose = SlidePoseStart;
	bIsSlidePoseActive = true;
	bIsUnSlidePoseActive = false;

	PreRoll();
}

void UCameraMovementComponent::ExitSlidePose()
{
	SlidePoseLerpTime = 0.f;
	PrevSlidePose = SlidePoseEnd;
	bIsUnSlidePoseActive = true;
	bIsSlidePoseActive = false;

	constexpr bool wasLeaningLeft = true;
	PreUnRoll(wasLeaningLeft);
}

void UCameraMovementComponent::OnLandedFromAir()
{
	bNeedsDip = true;
	DipLerpTime = 0.f;
}

void UCameraMovementComponent::OnSlideActionOccured(bool aIsSlideActive)
{
	if (aIsSlideActive)
		EnterSlidePose();
	else
		ExitSlidePose();
}

