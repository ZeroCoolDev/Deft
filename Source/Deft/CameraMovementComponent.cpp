#include "CameraMovementComponent.h"

#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "DeftLocks.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

//TODO: maybesplit this into 5 CameraMovementComponent classes: Bobble, Roll, Dip(Land), Pitch , Slide
TAutoConsoleVariable<bool> CVar_EnableBobble(TEXT("deft.camreafeatures.enable.Bobble"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnableRoll(TEXT("deft.camreafeatures.enable.Roll"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnableDip(TEXT("deft.camreafeatures.enable.Dip"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnablePitch(TEXT("deft.camreafeatures.enable.Pitch"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnableSlide(TEXT("deft.camreafeatures.enable.Slide"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);


UCameraMovementComponent::UCameraMovementComponent()
	: WalkBobbleCurve(nullptr)
	, DeftCharacter(nullptr)
	, DeftMovementComponent(nullptr)
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
	, SlideZPosLerpTime(0.f)
	, SlideZPosLerpTimeMax(0.f)
	, SlideZPosStart(0.f)
	, SlideZPosEnd(0.f)
	, PrevSlideZPos(0.f)
	, HighestSlideZPosTimeAchieved(0.f)
	, SlideRollEndOverride(0.f)
	, SlideRollLerpTimeMaxOverride(0.f)
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

	DeftCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (!DeftCharacter.IsValid())
		UE_LOG(LogTemp, Error, TEXT("Missing owner"));

	// Bobble Setup
	if (WalkBobbleCurve)
	{
		float unusedMin;
		WalkBobbleCurve->GetTimeRange(unusedMin, WalkBobbleMaxTime);
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Walk Bobble curve is invalid"));

	CameraTarget = DeftCharacter->FindComponentByClass<USpringArmComponent>();
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
	DeftMovementComponent = Cast<UDeftCharacterMovementComponent>(DeftCharacter->GetMovementComponent());
	if (DeftMovementComponent.IsValid())
	{
		DeftMovementComponent->OnLandedFromAir.AddUObject(this, &UCameraMovementComponent::OnLandedFromAir);
		DeftMovementComponent->OnSlideActionOccured.AddUObject(this, &UCameraMovementComponent::OnSlideActionOccured);
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Failed to get DeftCharacterMovementComponent"));

	// Slide Pose setup
	SlideZPosLerpTimeMax = 0.15f;
	SlideRollEndOverride = 20.f;
	SlideRollLerpTimeMaxOverride = 0.15f;
	SlideZPosStart = DeftCharacter->GetCapsuleComponent()->GetRelativeLocation().Z;
	SlideZPosEnd = 42.f;
}

void UCameraMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CVar_EnableBobble.GetValueOnGameThread())
		ProcessCameraBobble(DeltaTime);
	if (CVar_EnableRoll.GetValueOnGameThread())
		ProcessCameraRoll(DeltaTime);
	if (CVar_EnableDip.GetValueOnGameThread())
		ProcessCameraDip(DeltaTime);
	if (CVar_EnablePitch.GetValueOnGameThread())
		ProcessCameraPitch(DeltaTime);
	if (CVar_EnableSlide.GetValueOnGameThread())
		ProcessCameraSlide(DeltaTime);

	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::Green, FString::Printf(TEXT("%.2f"), CameraTarget->GetRelativeLocation().Z));

	PreviousInputVector = DeftCharacter->GetInputMoveVector();
}

void UCameraMovementComponent::ProcessCameraBobble(float aDeltaTime)
{
	if (bNeedsDip) // don't play dip and bobble at the same time otherwise they fight eachtoher
		return;

	if (!WalkBobbleCurve)
		return;

	if (!DeftCharacter->GetCharacterMovement()->IsMovingOnGround())
		return;

	if (DeftCharacter->GetVelocity() == FVector::ZeroVector)
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
	// Slide hijacks roll so don't fight over who's rolling during slide
	if (bIsSlideActive || bIsUnSlideActive)
		return;

	// Camera roll
	const FVector2D& inputVector = DeftCharacter->GetInputMoveVector();

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
	const float rollLerpMaxTime = bIsSlideActive ? SlideRollLerpTimeMaxOverride : RollLerpTimeMax;

	RollLerpTime += aDeltaTime;
	if (RollLerpTime > rollLerpMaxTime)
		RollLerpTime = rollLerpMaxTime;
	HighestRollTimeAchieved = RollLerpTime;

	// lerp!
	const float percent = RollLerpTime / rollLerpMaxTime;
	float roll = FMath::Lerp(RollLerpStart, bIsSlideActive ? SlideRollEndOverride : RollLerpEnd, percent);
	if (aIsLeaningLeft)
		roll *= -1.f;


	const FRotator rotation = DeftCharacter->GetController()->GetControlRotation();
	const FRotator newRotation(rotation.Pitch, rotation.Yaw, roll);
	DeftCharacter->GetController()->SetControlRotation(newRotation);
}

void UCameraMovementComponent::PreUnRoll(bool aWasLeaningLeft)
{
	const float rollLerpMaxTime = bIsUnSlideActive ? SlideRollLerpTimeMaxOverride : RollLerpTimeMax;

	// Time to unroll is faster than rolling so need to map time from roll to unroll range
	const float unrollLerpInRollRange = rollLerpMaxTime - HighestRollTimeAchieved;
	UnrollLerpTime = (unrollLerpInRollRange / rollLerpMaxTime) * (bIsUnSlideActive ? SlideRollLerpTimeMaxOverride : UnrollLerpTimeMax);

	bUnrollFromLeft = aWasLeaningLeft;
	RollLerpTime = 0.f; // reset roll lerp since we're unrolling so that the time isn't carried over to an different lean
	bNeedsUnroll = true;
	HighestRollTimeAchieved = 0.f;
}

void UCameraMovementComponent::UnRoll(float aDeltaTime)
{
	UnrollLerpTime += aDeltaTime;
	const float unrollLerpTime = bIsUnSlideActive ? SlideRollLerpTimeMaxOverride : UnrollLerpTimeMax;
	if (UnrollLerpTime > unrollLerpTime)
	{
		UnrollLerpTime = unrollLerpTime;
		bNeedsUnroll = false; // this is the last unroll frame we need
	}

	// lerp!
	const float percent = UnrollLerpTime / unrollLerpTime;
	float unroll = FMath::Lerp(bIsUnSlideActive ? SlideRollEndOverride : RollLerpEnd, RollLerpStart, percent);
	if (bUnrollFromLeft)
		unroll *= -1.f;

	const FRotator rotation = DeftCharacter->GetController()->GetControlRotation();
	const FRotator newRotation(rotation.Pitch, rotation.Yaw, unroll);
	DeftCharacter->GetController()->SetControlRotation(newRotation);
}

void UCameraMovementComponent::ProcessCameraDip(float aDeltaTime)
{
	// TODO: this might have an bug if we enter sliding or jump again before the dip is complete?
	if (!bNeedsDip)
		return;

	if (!CameraTarget.IsValid())
		return;

	const float prevDipLerpTime = DipLerpTime;
	DipLerpTime += aDeltaTime;
	if (DipLerpTime > DipLerpTimeMax)
	{
		if (prevDipLerpTime < DipLerpTimeMax)
		{
			// Last frame making sure we hit the max and min
			DipLerpTime = DipLerpTimeMax;
		}
		else
		{
			bNeedsDip = false;
			return;
		}
	}

	const float dipCurveVal = LandedFromAirDipCuve->GetFloatValue(DipLerpTime);
	const float dipCurveValDelta = dipCurveVal - PrevDipVal;

	PrevDipVal = dipCurveVal;

	const FVector cameraLocation = CameraTarget->GetRelativeLocation();
	const FVector newLocation = cameraLocation - FVector(0.f, 0.f, dipCurveValDelta);

	CameraTarget->SetRelativeLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void UCameraMovementComponent::ProcessCameraPitch(float aDeltaTime)
{
	const bool isBackwardsInput = DeftCharacter->GetInputMoveVector().Y < 0.f;
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

		DeftCharacter->AddControllerPitchInput(-pitchDelta);

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
		DeftCharacter->AddControllerPitchInput(unPitchDetlta);

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

void UCameraMovementComponent::ProcessCameraSlide(float aDeltaTime)
{
	if (!bIsSlideActive && !bIsUnSlideActive)
		return;

	if (!CameraTarget.IsValid())
		return;

	if (bIsSlideActive || bIsUnSlideActive)
	{
		if (bIsSlideActive)
		{
			constexpr bool isLeaningLeft = true;
			Roll(aDeltaTime, isLeaningLeft);
		}
		else
			UnRoll(aDeltaTime);

		const float prevSlideLerpTime = SlideZPosLerpTime;
		SlideZPosLerpTime += aDeltaTime;
		if (SlideZPosLerpTime > SlideZPosLerpTimeMax)
		{
			if (prevSlideLerpTime < SlideZPosLerpTimeMax)
			{
				// Last frame making sure we hit the max and min
				SlideZPosLerpTime = SlideZPosLerpTimeMax;
			}
			else
			{
				if (bIsSlideActive)
					return;

				if (bIsUnSlideActive)
				{
					ExitSlide();
					return;
				}
			}
		}
		HighestSlideZPosTimeAchieved = SlideZPosLerpTime;

		// TODO: This is a little messy, cleanup rather than having to do a buncha terinaries

		// lerp!
		const float percent = SlideZPosLerpTime / SlideZPosLerpTimeMax;
		// lerp either moves camera from higher to lower (sliding) or lower to higher (done sliding)
		const float newCameraZPos = bIsSlideActive ? FMath::Lerp(SlideZPosStart, SlideZPosEnd, percent) : FMath::Lerp(SlideZPosEnd, SlideZPosStart, percent);
		const float cameraZPosDelta = FMath::Abs(newCameraZPos - PrevSlideZPos);

		const FVector cameraLocation = CameraTarget->GetRelativeLocation();
		const FVector destinationLocation = cameraLocation - FVector(0.f, 0.f, bIsSlideActive ? cameraZPosDelta : -cameraZPosDelta);
		CameraTarget->SetRelativeLocation(destinationLocation);

		UE_LOG(LogTemp, Warning, TEXT("%.2f%%:, lerpRes %.2f, cam src->dest %.2f -> %.2f"), percent, newCameraZPos, cameraLocation.Z, destinationLocation.Z);
		PrevSlideZPos = newCameraZPos;
	}
}

void UCameraMovementComponent::EnterSlide()
{
	DeftLocks::IncrementSlideLockRef();

	HighestSlideZPosTimeAchieved = 0.f;
	SlideZPosLerpTime = 0.f;
	PrevSlideZPos = SlideZPosStart;
	bIsSlideActive = true;
	bIsUnSlideActive = false;

	PreRoll();
}

void UCameraMovementComponent::UnSlide()
{
	// unSliding just inverts the range, so wherever we made it to when sliding (ex 2.5/3 for a total of slide time = 2.5)
	// we need to start unSliding at 0.5/3 so the total unSlide time = 2.5
	SlideZPosLerpTime = SlideZPosLerpTimeMax - HighestSlideZPosTimeAchieved;

	// Setting previous to the current lerp time value so we don't have a huge jump for 1 frame since the range is reversed from sliding
	const float percent = SlideZPosLerpTime / SlideZPosLerpTimeMax;
	PrevSlideZPos = FMath::Lerp(SlideZPosEnd, SlideZPosStart, percent);

	bIsUnSlideActive = true;
	bIsSlideActive = false;

	constexpr bool wasLeaningLeft = true;
	PreUnRoll(wasLeaningLeft);
}

void UCameraMovementComponent::ExitSlide()
{
	SlideZPosLerpTime = 0.f;
	HighestSlideZPosTimeAchieved = 0.f;
	bIsSlideActive = false;
	bIsUnSlideActive = false;

	DeftLocks::DecrementSlideLockRef();
}

void UCameraMovementComponent::OnLandedFromAir()
{
	bNeedsDip = true;
	DipLerpTime = 0.f;
}

void UCameraMovementComponent::OnSlideActionOccured(bool aIsSlideActive)
{
	if (aIsSlideActive)
		EnterSlide();
	else
		UnSlide();
}

