#include "CameraMovementComponent.h"

#include "ClimbComponent.h"
#include "DeftCharacterMovementComponent.h"
#include "DeftPlayerCharacter.h"
#include "DeftLocks.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

//TODO: maybesplit this into 5 CameraMovementComponent classes: Bobble, Roll, Dip(Land), Pitch , Slide

TAutoConsoleVariable<bool> CVar_EnableBobble(TEXT("deft.enable.camera.Bobble"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnableLean(TEXT("deft.enable.camera.Lean"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnableDip(TEXT("deft.enable.camera.Dip"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnablePitch(TEXT("deft.enable.camera.Pitch"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_EnableSlide(TEXT("deft.enable.camera.Slide"), true, TEXT("true = enabled, false = disabled"), ECVF_Cheat);

TAutoConsoleVariable<bool> CVar_DebugEnable(TEXT("deft.debug.camera"), false, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugBobble(TEXT("deft.camera.debug.Bobble"), false, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugLean(TEXT("deft.debug.camera.Lean"), false, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugDip(TEXT("deft.debug.camera.Dip"), false, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugPitch(TEXT("deft.debug.camera.Pitch"), false, TEXT("true = enabled, false = disabled"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugCameraSlide(TEXT("deft.debug.camera.Slide"), false, TEXT("true = enabled, false = disabled"), ECVF_Cheat);


UCameraMovementComponent::UCameraMovementComponent()
	: WalkBobbleCurve(nullptr)
	, LandedFromAirDipCuve(nullptr)
	, DefaultCameraRelativeZPosition(0.f)
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
	{
		UE_LOG(LogTemp, Error, TEXT("Missing owner"));
		return;
	}

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

	DefaultCameraRelativeZPosition = CameraTarget->GetRelativeLocation().Z;

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
	if (CVar_EnableLean.GetValueOnGameThread())
		ProcessCameraRoll(DeltaTime);
	if (CVar_EnableDip.GetValueOnGameThread())
		ProcessCameraDip(DeltaTime);
	if (CVar_EnablePitch.GetValueOnGameThread())
		ProcessCameraPitch(DeltaTime);
	if (CVar_EnableSlide.GetValueOnGameThread())
		ProcessCameraSlide(DeltaTime);

	PreviousInputVector = DeftCharacter->GetInputMoveVector();

#if !UE_BUILD_SHIPPING
	DrawDebug();
#endif//!UE_BUILD_SHIPPING
}

void UCameraMovementComponent::ProcessCameraBobble(float aDeltaTime)
{
	if (!WalkBobbleCurve)
		return;

	if (!CameraTarget.IsValid())
		return;

	if (WalkBobbleTime >= WalkBobbleMaxTime)
		WalkBobbleTime = 0.f;

	const bool shouldStopBobble = bNeedsDip ||
		!DeftCharacter->GetCharacterMovement()->IsMovingOnGround() ||
		DeftCharacter->GetVelocity() == FVector::ZeroVector;
#if !UE_BUILD_SHIPPING
	bShouldStopBobble = shouldStopBobble;
#endif//!UE_BUILD_SHIPPING

	if (shouldStopBobble)
	{
		// Only stop bobble on a completed cycle (i.e. camera position is restored fully) otherwise let it continue this last cycle
		if (WalkBobbleTime == 0.f)
		{
			return;
		}
		else if (WalkBobbleTime < WalkBobbleMaxTime / 2.f)
		{
			// We were on the ascent (since apex is exact middle of MaxTime)
			// find corresponding descent time so we just "undo" whatever bobble has happened instead of played an entire cycle (which looks bad)
			// Ex:
			// ascent time = 0.15,	max = 0.4 ==> descent time = 0.25
			// ascent time = 0.078,	max = 0.4 ==> descent time = 0.322
			WalkBobbleTime = WalkBobbleMaxTime - WalkBobbleTime;
		}
	}

	WalkBobbleTime += aDeltaTime;
	const float bobbleCurveVal = WalkBobbleCurve->GetFloatValue(WalkBobbleTime);
	const float bobbleCurveValDelta = bobbleCurveVal - PrevWalkBobbleVal;

	PrevWalkBobbleVal = bobbleCurveVal;

	const FVector cameraLocation = CameraTarget->GetRelativeLocation();
	const FVector newLocation = cameraLocation - FVector(0.f, 0.f, bobbleCurveValDelta);

	CameraTarget->SetRelativeLocation(newLocation, false, nullptr, ETeleportType::TeleportPhysics);

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

#if !UE_BUILD_SHIPPING
	bIsLeaningLeft = isLeaningLeft;
	bIsLeaningRight = isLeaningRight;
#endif//!UE_BUILD_SHIPPING

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
	if (!bNeedsDip)
		return;

	if (DeftLocks::IsCameraMovementDipLocked())
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
		const float prevPitchLerpTime = PitchLerpTime;
		PitchLerpTime += aDeltaTime;
		if (PitchLerpTime > PitchLerpTimeMax)
		{
			if (prevPitchLerpTime < PitchLerpTimeMax)
				PitchLerpTime = PitchLerpTimeMax; // Last frame of pitch add needed to make sure we hit the max and min
			else
				return; // cease adding pitch, but don't disable yet because player is still moving backwards
		}
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
	if (!DeftLocks::IsCameraMovementDipLocked())
	{
		bNeedsDip = true;
		DipLerpTime = 0.f;
	}
}

void UCameraMovementComponent::OnSlideActionOccured(bool aIsSlideActive)
{
	if (aIsSlideActive)
		EnterSlide();
	else
		UnSlide();
}

#if !UE_BUILD_SHIPPING
void UCameraMovementComponent::DrawDebug()
{
	if (CVar_DebugEnable.GetValueOnGameThread())
	{
		FString movementDebug;
		movementDebug += FString::Printf(TEXT("-Movement-\n\tInput: %s\n\tPrev Input: %s\n\tSpeed: %.2f\n\tInput Locked: %d")
			, *DeftCharacter->GetInputMoveVector().ToString()
			, *PreviousInputVector.ToString()
			, DeftCharacter->GetVelocity().Length()
			, DeftLocks::IsInputLocked());

		FString cameraDebug;
		cameraDebug += FString::Printf(TEXT("\n-Camera-\n\tZ Height: %.2f\n\tRotation: %s")
			, CameraTarget->GetRelativeLocation().Z
			, *DeftCharacter->GetController()->GetControlRotation().ToString());

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 0.005f, DeftLocks::IsInputLocked() ? FColor::Red : FColor::White, *movementDebug, false);
			GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, *cameraDebug, false);
		}
	}

	if (CVar_DebugBobble.GetValueOnGameThread())
		DrawDebugBobble();
	if (CVar_DebugLean.GetValueOnGameThread())
		DrawDebugRoll();
	if (CVar_DebugDip.GetValueOnGameThread())
		DrawDebugDip();
	if (CVar_DebugPitch.GetValueOnGameThread())
		DrawDebugPitch();
	if (CVar_DebugCameraSlide.GetValueOnGameThread())
		DrawDebugSlide();
}

void UCameraMovementComponent::DrawDebugBobble()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bShouldStopBobble ? FColor::Red : FColor::Green, FString::Printf(TEXT("\n-Bobble-\n\tLerp: %.2f / %.2f")
			, WalkBobbleTime
			, WalkBobbleMaxTime), false);
	}
}

void UCameraMovementComponent::DrawDebugRoll()
{
	if (GEngine)
	{
		const FString roll = FString::Printf(TEXT("\tRolling: %.2f / %.2f\n\tFarthest Roll Time: %.2f\n\tRoll Range [%.2f, %.2f]")
			, RollLerpTime
			, RollLerpTimeMax
			, HighestRollTimeAchieved
			, RollLerpStart
			, bIsUnSlideActive ? SlideRollEndOverride : RollLerpEnd);

		const FString unroll = FString::Printf(TEXT("\tUnrolling: %.2f / %.2f\n\tUnroll Range [%.2f, %.2f]")
			, UnrollLerpTime
			, UnrollLerpTimeMax
			, bIsUnSlideActive ? SlideRollEndOverride : RollLerpEnd
			, RollLerpStart);

		GEngine->AddOnScreenDebugMessage(-1, 0.005f, (bIsSlideActive || bIsUnSlideActive) ? FColor::Yellow : (bIsLeaningLeft || bIsLeaningRight || bNeedsUnroll) ? FColor::Green : FColor::White, FString::Printf(TEXT("\n-Lean-")), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, (bIsSlideActive || bIsUnSlideActive) ? FColor::Yellow : (bIsLeaningLeft || bIsLeaningRight) ? FColor::Cyan : (bNeedsUnroll ? FColor::Magenta : FColor::White), FString::Printf(TEXT("\tRot. Roll: %.2f"), DeftCharacter->GetController()->GetControlRotation().Roll), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsSlideActive ? FColor::Yellow : (bIsLeaningLeft || bIsLeaningRight) ? FColor::Cyan : FColor::Red, *roll, false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsUnSlideActive ? FColor::Yellow : bNeedsUnroll ? FColor::Magenta : FColor::Red, *unroll, false);
	}
}

void UCameraMovementComponent::DrawDebugDip()
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bNeedsDip ? FColor::Green : FColor::Red, FString::Printf(TEXT("\n-Dip-\n\tLerp: %.2f / %.2f")
			, DipLerpTime
			, DipLerpTimeMax), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, FString::Printf(TEXT("\tCam Z Origin: %.2f"), DefaultCameraRelativeZPosition), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bNeedsDip ? FColor::Green : FColor::Red, FString::Printf(TEXT("\tCam Z delta: %.2f\n\tCam Z Pos: %.2f")
			, FMath::Abs(DefaultCameraRelativeZPosition - CameraTarget->GetRelativeLocation().Z)
			, CameraTarget->GetRelativeLocation().Z), false);
	}
}

void UCameraMovementComponent::DrawDebugPitch()
{
	if (GEngine)
	{
		const FString pitch = FString::Printf(TEXT("\tTilting: %.2f / %.2f\n\tFarthest Tilt Time: %.2f\n\tTilt Range [%.2f, %.2f]")
			, PitchLerpTime
			, PitchLerpTimeMax
			, HighestTiltTimeAchieved
			, PitchStart
			, PitchEnd);

		const FString unpitch = FString::Printf(TEXT("\tUntilting: %.2f / %.2f\n\tUntilt Range [%.2f, %.2f]")
			, UnPitchLerpTime
			, UnPitchLerpTimeMax
			, PitchEnd
			, PitchStart);

		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsPitchActive || bIsUnPitchActive ? FColor::Green : FColor::White, FString::Printf(TEXT("\n-Tilt-")), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsPitchActive ? FColor::Cyan : (bIsUnPitchActive ? FColor::Magenta : FColor::White), FString::Printf(TEXT("\tRot. Pitch: %.2f"), DeftCharacter->GetController()->GetControlRotation().Pitch), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsPitchActive ? FColor::Cyan : FColor::Red, *pitch, false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsUnPitchActive ? FColor::Magenta : FColor::Red, *unpitch, false);
	}
}

void UCameraMovementComponent::DrawDebugSlide()
{
	if (GEngine)
	{
		const FString pitch = FString::Printf(TEXT("\tSliding: %.2f / %.2f\n\tFarthest Slide Time: %.2f\n\tSlide Range [%.2f, %.2f]")
			, SlideZPosLerpTime
			, SlideZPosLerpTimeMax
			, HighestSlideZPosTimeAchieved
			, SlideZPosStart
			, SlideZPosEnd);

		const FString unpitch = FString::Printf(TEXT("\tUnsliding: %.2f / %.2f\n\tUnslide Range [%.2f, %.2f]")
			, PrevSlideZPos
			, SlideZPosLerpTimeMax
			, SlideZPosEnd
			, SlideZPosStart);

		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsSlideActive || bIsUnSlideActive ? FColor::Green : FColor::White, FString::Printf(TEXT("\n-Slide-")), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, FString::Printf(TEXT("\tCam Z Origin: %.2f"), DefaultCameraRelativeZPosition), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, (bIsSlideActive || bIsUnSlideActive) ? FColor::Green : FColor::Red, FString::Printf(TEXT("\tCam Z delta: %.2f\n\tCam Z Pos: %.2f")
			, FMath::Abs(DefaultCameraRelativeZPosition - CameraTarget->GetRelativeLocation().Z)
			, CameraTarget->GetRelativeLocation().Z), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsSlideActive ? FColor::Cyan : (bIsUnSlideActive ? FColor::Magenta : FColor::White), FString::Printf(TEXT("\tRot. Roll: %.2f"), DeftCharacter->GetController()->GetControlRotation().Roll), false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsSlideActive ? FColor::Cyan : FColor::Red, *pitch, false);
		GEngine->AddOnScreenDebugMessage(-1, 0.005f, bIsUnSlideActive ? FColor::Magenta : FColor::Red, *unpitch, false);
	}
}

#endif//!UE_BUILD_SHIPPING
