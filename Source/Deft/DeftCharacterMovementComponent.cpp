#include "DeftCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "ClimbComponent.h"
#include "DeftPlayerCharacter.h"
#include "DeftLocks.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"


TAutoConsoleVariable<int> CVar_FeatureJumpCurve(TEXT("deft.feature.jump"), 1, TEXT("1=use custom jump curve logic, 0=use engine jump logic"), ECVF_Cheat);
TAutoConsoleVariable<int> CVar_Feature_SlideMode(TEXT("deft.feature.slide"), 1, TEXT("0=slide distance is determined by entering velocity, 1=slide distance is consistent regardless of entering velocity"), ECVF_Cheat);

TAutoConsoleVariable<bool> CVar_DebugLocks(TEXT("deft.debug.locks"), false, TEXT("show debugging for locks"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugJump(TEXT("deft.debug.jump"), false, TEXT("draw debug for jumping"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugSlide(TEXT("deft.debug.slide"), false, TEXT("draw debug for sliding"), ECVF_Cheat);
TAutoConsoleVariable<bool> CVar_DebugFall(TEXT("deft.debug.fall"), false, TEXT("draw debug for falling"), ECVF_Cheat);

UDeftCharacterMovementComponent::UDeftCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, JumpCurve(nullptr)
	, JumpFallCurve(nullptr)
	, NonJumpFallCurve(nullptr)
	, SlideCurve(nullptr)
	, SlideDirection(FVector::ZeroVector)
	, SlideJumpAdditive(FVector::ZeroVector)
	, FallCurveToUse(nullptr)
	, JumpTime(0.f)
	, PrevJumpTime(0.f)
	, JumpCurveStartTime(0.f)
	, JumpCurveMaxTime(0.f)
	, PrevJumpCurveVal(0.f)
	, JumpApexTime(0.f)
	, JumpApexHeight(0.f)
	, FallTime(0.f)
	, PrevFallCurveVal(0.f)
	, SlideTime(0.f)
	, SlideSpeedMax(0.f)
	, SlideMaxTime(0.f)
	, SlideCurveStartTime(0.f)
	, SlideCurveMaxTime(0.f)
	, SlideMinimumStartTime(0.f)
	, SlideJumpSpeedMod(0.f)
	, SlideJumpSpeedModMax(0.f)
	, ImpulseFallDelay(0.f)
	, ImpulseFallDelayMax(0.f)
	, bIsJumping(false)
	, bIsValidJumpCurve(false)
	, bIsFalling(false)
	, bIsSliding(false)
	, bIsInImpulse(false)
{
}

void UDeftCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UClimbComponent* climbComponent = CharacterOwner->FindComponentByClass<UClimbComponent>())
		climbComponent->OnLedgeUpDelegate.AddUObject(this, &UDeftCharacterMovementComponent::OnClimbActionLedgeUp);

	bIsFalling = false;
	bIsJumping = false;

	if (JumpCurve)
	{
		JumpCurve->GetTimeRange(JumpCurveStartTime, JumpCurveMaxTime);

		float minHeight;
		JumpCurve->GetValueRange(minHeight, JumpApexHeight);

		if (!FindJumpApexTime(JumpApexTime))
			UE_LOG(LogTemp, Error, TEXT("Invalid Jump Curve, no Apex found"));

		UE_LOG(LogTemp, Warning, TEXT("Jump Curve Time (min,max): (%f, %f)"), JumpCurveStartTime, JumpCurveMaxTime);
		UE_LOG(LogTemp, Warning, TEXT("Jump Apex Height %f at time %f"), JumpApexTime, JumpApexHeight);
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Missing Jump Curve"));


	if (!JumpFallCurve)
		UE_LOG(LogTemp, Error, TEXT("Missing jump Fall Curve"));

	if (!NonJumpFallCurve)
		UE_LOG(LogTemp, Error, TEXT("Missing non-jump Fall Curve"));

	if (SlideCurve)
	{
		SlideCurve->GetTimeRange(SlideCurveStartTime, SlideCurveMaxTime);
		UE_LOG(LogTemp, Warning, TEXT("Slide Curve Time (min,max): (%f, %f)"), SlideCurveStartTime, SlideCurveMaxTime);
	}
	else 
		UE_LOG(LogTemp, Error, TEXT("Missing Slide Curve"));

	SlideSpeedMax = 1200.f;
	SlideMaxTime = 0.55f;

	SlideMinimumStartTime = 0.3;
	SlideJumpSpeedModMax = 4.f;

	ImpulseFallDelayMax = 2.f;
}

void UDeftCharacterMovementComponent::TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction)
{
	Super::TickComponent(aDeltaTime, aTickType, aThisTickFunction);
	
	ProcessJumping(aDeltaTime);
	ProcessFalling(aDeltaTime);
	ProcessSliding(aDeltaTime);

#if !UE_BUILD_SHIPPING
	DrawDebug();
#endif //!UE_BUILD_SHIPPING
}

bool UDeftCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("Jump curve disabled"));
		return Super::DoJump(bReplayingMoves);
	}
#endif//!UE_BUILD_SHIPPING

	if (bIsJumping)
		return false;

	if (CharacterOwner && CharacterOwner->CanJump()) // TODO: 'CanJump()' may have to be overridden if we wanna allow double jump stuff
	{
		// Don't jump if we can't move up/down
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{

			// Take into account any sliding additives
			SlideJumpAdditive = FVector::ZeroVector;
			if (bIsSliding)
			{
				SlideJumpAdditive = SlideDirection * SlideJumpSpeedMod;
				StopSlide();
			}

			// Ignore gravity but keep UE air control
			SetMovementMode(MOVE_Flying);

			bIsJumping = true;
			bIsFalling = false;

			JumpTime = JumpCurveStartTime;

			PrevJumpTime = JumpTime;
			PrevJumpCurveVal = JumpCurve->GetFloatValue(JumpTime);

#if !UE_BUILD_SHIPPING
			Debug_JumpHeightApex = 0.f;
#endif//!UE_BUILD_SHIPPING

			return true;
		}
	}
	
	return false;
}

bool UDeftCharacterMovementComponent::IsFalling() const
{
	// overriding default engine IsFalling() for animation reasons since we logically are in MOVE_Flying during any Jump or Fall
	// however animations may need to know if we're falling
#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
		return Super::IsFalling();
#endif
	// note: This causes horizontal collisions to break when jumping or falling into collision
	// TODO: investigate cause, my assumption is that the engine is colliding which puts us into MOVE_Walking, then it sees it's IsFalling() (because of our override)
	// and puts us in MOVE_Falling, and the cycle repeats
	// Can be investigated
	return Super::IsFalling() || bIsJumping || bIsFalling;
}

bool UDeftCharacterMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() && (IsMovingOnGround() || IsFalling() || bIsSliding);
}

bool UDeftCharacterMovementComponent::CanCrouchInCurrentState() const
{
	if (!CanEverCrouch())
		return false;

	return (IsFalling() || IsMovingOnGround() || bIsSliding) && UpdatedComponent && !UpdatedComponent->IsSimulatingPhysics();
}

void UDeftCharacterMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (MovementMode == MOVE_Flying)
		bCrouchMaintainsBaseLocation = true;
}

bool UDeftCharacterMovementComponent::CanStepUp(const FHitResult& Hit) const
{
	if (bIsJumping || bIsFalling)
		return false;

	return Super::CanStepUp(Hit);
}

void UDeftCharacterMovementComponent::OnClimbActionLedgeUp(bool aIsStarted)
{
	if (aIsStarted)
	{
		bIsJumping = false;
		bIsSliding = false;
		bIsFalling = false;

		SetMovementMode(MOVE_Flying);
	}
	else
		SetCustomFallingMode();
}

void UDeftCharacterMovementComponent::ProcessJumping(float aDeltaTime)
{
	bWasJumpingLastFrame = bIsJumping;

	if (!JumpCurve || !bIsJumping)
		return;

	JumpTime += aDeltaTime;
	if (JumpTime <= JumpCurveMaxTime)
	{
		// Get the new character location
		float jumpCurveVal = JumpCurve->GetFloatValue(JumpTime);

		// Make sure that the character always reaches the jump apex height and notify character about reaching apex
		bool isJumpApexReached = PrevJumpTime < JumpApexTime && JumpTime > JumpApexTime;
		if (isJumpApexReached)
		{
			JumpTime = JumpApexTime;
			jumpCurveVal = JumpApexHeight;
		}

		PrevJumpTime = JumpTime;

		const float jumpCurveValDelta = jumpCurveVal - PrevJumpCurveVal;
		PrevJumpCurveVal = jumpCurveVal;

		// when in MOVE_Flying the base character movement component will move the character based off velocity
		// yet we want to move based off the curve position
		// TODO(optional): consider using custom and making specific movement modes for these custom jump & falling
		// which means we'll have to handle input when the character is in the air just as a heads up
		Velocity.Z = 0.f;
		float yVelocity = jumpCurveValDelta / aDeltaTime; //note: velocity = distance / time

		const FVector actorLocation = GetActorLocation();
		FVector destinationLocation = actorLocation + FVector(0.f, 0.f, jumpCurveValDelta) + SlideJumpAdditive;

		// Check roof collision if character is moving up
		if (yVelocity > 0.f)
		{
			FCollisionQueryParams roofCheckCollisionParams;
			roofCheckCollisionParams.AddIgnoredActor(CharacterOwner);
			// TODO: could cache this since the capsule shape isn't going to change dynamically 
			const UCapsuleComponent* capsulComponent = CharacterOwner->GetCapsuleComponent();
			FCollisionShape capsulShape = FCollisionShape::MakeCapsule(capsulComponent->GetScaledCapsuleRadius(), capsulComponent->GetScaledCapsuleHalfHeight());

			FHitResult roofHitResult;
			const bool bIsBlockingHit = GetWorld()->SweepSingleByProfile(roofHitResult, actorLocation, destinationLocation, CharacterOwner->GetActorRotation().Quaternion(), capsulComponent->GetCollisionProfileName(), capsulShape, roofCheckCollisionParams);
			if (bIsBlockingHit)
			{
				// To be sure we actually hit a roof and not just intersected with an object due to slide + jump speed moving the component too far
				// do another check only taking into account the destination's vertical location
				const FVector destVertOnly = FVector(actorLocation.X, actorLocation.Y, destinationLocation.Z);
				const bool bStillCollidedVertically = GetWorld()->SweepSingleByProfile(roofHitResult, actorLocation, destVertOnly, CharacterOwner->GetActorRotation().Quaternion(), capsulComponent->GetCollisionProfileName(), capsulShape, roofCheckCollisionParams);

				// Now we are confident we actually hit a roof
				if (bStillCollidedVertically)
				{
					// Roof collision hit
					SetCustomFallingMode();

					bIsJumping = false;
					CharacterOwner->StopJumping();

					// Reset vertical velocity to let gravity do the work
					Velocity.Z = 0.f;

					// Take character to a safe location where its not hitting roof
					// TODO: this can be improved by using the impact location and using that
					destinationLocation = actorLocation;
				}
			}

#if !UE_BUILD_SHIPPING
			// accumulating distance traveled UPWARDS to find the apex
			Debug_JumpHeightApex += (destinationLocation.Z - actorLocation.Z);
#endif //!UE_BUILD_SHIPPING
		}

		// There is no floor checks while in MOVE_Flying so do it manually
		bool landedOnFloor = false;
		if (yVelocity < 0.f)
		{
			const FVector capsulLoc = UpdatedComponent->GetComponentLocation();
			FFindFloorResult floorResult;
			if (FindFloorBySweep(floorResult, capsulLoc, destinationLocation))
			{
				const float floorDistance = floorResult.GetDistanceToFloor();
				if (FMath::Abs(jumpCurveValDelta) > floorDistance)
					destinationLocation = capsulLoc - FVector(0.f, 0.f, floorDistance);

				SetMovementMode(MOVE_Walking);

				bIsJumping = false;
				CharacterOwner->StopJumping();
				landedOnFloor = true;
			}
		}

		// Move the actual capsule component in the world
		FLatentActionInfo latentInfo;
		latentInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLocation, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Type::Move, latentInfo);

		// Notifying for animation support. Nothing in code is actively using this atm
		if (isJumpApexReached && bNotifyApex)
			NotifyJumpApex();

		if (landedOnFloor)
			OnLandedFromAir.Broadcast();

	}
	else
	{
		// Must invalidate before trying to ResetJumpState if we hit a floor because it requires the player not to be IsFalling()
		// otherwise we're falling and bIsJumping should still be invalid
		bIsJumping = false;

		// reached the end of the jump curve, check for a floor otherwise we're falling
		const FVector capsulLoc = UpdatedComponent->GetComponentLocation();
		FFindFloorResult floorResult;
		FindFloor(capsulLoc, floorResult, false);
		if (floorResult.IsWalkableFloor() && IsValidLandingSpot(capsulLoc, floorResult.HitResult))
		{
			SetMovementMode(MOVE_Walking);
			OnLandedFromAir.Broadcast();

			//UE_LOG(LogTemp, Warning, TEXT("finished jump last frame - floor Collision"));
		}
		else
		{
			//UE_LOG(LogTemp, Warning, TEXT("finished jump last frame - falling mode"));
			SetCustomFallingMode();
		}

		CharacterOwner->StopJumping();
	}
}

void UDeftCharacterMovementComponent::ProcessFalling(float aDeltaTime)
{
	if (bIsInImpulse)
	{
		ProcessImpulseFallDelay(aDeltaTime);
		return;
	}

	if (bIsFalling)
	{
		if (!FallCurveToUse)
		{
			UE_LOG(LogTemp, Error, TEXT("Missing FallCurveToUse!"));
			return;
		}

		FallTime += aDeltaTime;

		const float fallCurveVal = FallCurveToUse->GetFloatValue(FallTime);
		const float fallCurveValDelta = fallCurveVal - PrevFallCurveVal;
		PrevFallCurveVal = fallCurveVal;

		Velocity.Z = 0.f;

		const FVector capsuleLocation = UpdatedComponent->GetComponentLocation();
		FVector destinationLocation = capsuleLocation + FVector(0.f, 0.f, fallCurveValDelta);

		bool landedOnFloor = false;
		FFindFloorResult floorResult;
		if (FindFloorBySweep(floorResult, capsuleLocation, destinationLocation))
		{
			const float floorDistance = floorResult.GetDistanceToFloor();
			if (FMath::Abs(fallCurveValDelta) > floorDistance)
				destinationLocation = capsuleLocation - FVector(0.f, 0.f, floorDistance);

			bIsFalling = false;

			// Stopping the character and canceling all the movement carried from before the jump/fall
			// note: Remove if you want to carry the momentum
			Velocity = FVector::ZeroVector;

			SetMovementMode(MOVE_Walking);
			landedOnFloor = true;

			//DrawDebugCapsule(GetWorld(), destinationLocation, CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius(), CharacterOwner->GetActorRotation().Quaternion(), FColor::Yellow, false, 5.f);
		}

		FLatentActionInfo latentInfo;
		latentInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLocation, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);

		if (landedOnFloor)
			OnLandedFromAir.Broadcast();
	}
	else if (MovementMode == EMovementMode::MOVE_Falling)
	{
		// Dropping down from ledge while walking should use our custom fall logic
		// TODO: need a fall curve for when not jumping I think otherwise it's a little too aggressive of a fall

#if !UE_BUILD_SHIPPING
		// note: not doing it during engine jump since engine jump uses falling
		if (!IsJumpCurveEnabled())
			return;
#endif

		//UE_LOG(LogTemp, Warning, TEXT("UE MOVE_Falling"));
		SetCustomFallingMode();
	}
}

void UDeftCharacterMovementComponent::ProcessSliding(float aDeltaTime)
{
	if (!bIsSliding)
		return;

	if (!SlideCurve)
		return;

	if (bIsJumping || bIsFalling)
	{
		StopSlide();
		return;
	}

	float slideMaxTime = 0.f;
	if (CVar_Feature_SlideMode.GetValueOnGameThread() == 0) // Velocity based slide distance
		slideMaxTime = SlideCurveMaxTime;
	else if (CVar_Feature_SlideMode.GetValueOnGameThread() == 1) // Constant slide distance
		slideMaxTime = SlideMaxTime;

	// move component based off curve (increasing speed essentially)
	const float prevSlideTime = SlideTime;
	SlideTime += aDeltaTime;
	if (SlideTime > slideMaxTime)
	{
		if (prevSlideTime < slideMaxTime)
		{
			// Last frame making sure we hit the max and min
			SlideTime = slideMaxTime;
		}
		else
		{
			StopSlide();
			return;
		}
	}

	float slideSpeed = 0.f;
	if (CVar_Feature_SlideMode.GetValueOnGameThread() == 0) // Velocity based slide distance
		slideSpeed = SlideCurve->GetFloatValue(SlideTime);
	else if (CVar_Feature_SlideMode.GetValueOnGameThread() == 1) // Constant slide distance
		slideSpeed = SlideSpeedMax;

	const FVector actorLocation = CharacterOwner->GetActorLocation();
	const FVector destinationLocation = actorLocation + (SlideDirection * slideSpeed * aDeltaTime);

	FLatentActionInfo latentInfo;
	latentInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLocation, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);

#if !UE_BUILD_SHIPPING
	Debug_SlideVal = slideSpeed;
	Debug_TimeInSlide = SlideTime;
#endif
}

void UDeftCharacterMovementComponent::ProcessImpulseFallDelay(float aDeltaTime)
{
	if (!bIsInImpulse)
		return;

	ImpulseFallDelay += aDeltaTime;
	if (ImpulseFallDelay >= ImpulseFallDelayMax)
		bIsInImpulse = false;
}

// TODO: there is a bug where if you're walking into collision that you _can_ slide under, when you slide you'll be displaced horizontally 
// as if it were an impassible wall instead of sliding under it
void UDeftCharacterMovementComponent::DoSlide()
{
	// Don't allow sliding while currently sliding
	if (bIsSliding)
		return;

	if (DeftLocks::IsSlideLocked())
		return;

	if (Velocity == FVector::ZeroVector)
		return;

	// Only allow sliding while on the ground
	if (!IsMovingOnGround())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot slide because not moving on ground"));
		return;
	}

	DeftLocks::IncrementInputLockRef();

	SetMovementMode(MOVE_Flying);

	bIsSliding = true;

#if !UE_BUILD_SHIPPING
	Debug_SlideStartPos = CharacterOwner->GetActorLocation();
	Debug_SlideEndPos = Debug_SlideStartPos;
	Debug_TimeInSlide = 0.f;
#endif //!UE_BUILD_SHIPPING

	// slide in the direction of the player's last velocity but independent of it's speed (which is constant during slide excluding jump)
	SlideDirection = Velocity.GetSafeNormal();

	// Force backwards movement to be much less since sliding backwards is harder than forward
	const float minSpeedPercent = 0.5f;
	float speedPercent = 1.f;

	if (CVar_Feature_SlideMode.GetValueOnGameThread() == 0) // Velocity based slide distance
	{
		// min speed percent makes sure we can still slide even at very low velocities
		speedPercent = FMath::Max(minSpeedPercent, Velocity.Length() / GetMaxSpeed());
	}

	// Force backwards movement to be much less since sliding backwards is harder than forward
	if (Cast<ADeftPlayerCharacter>(CharacterOwner)->GetInputMoveVector().Y < 0.f)
		speedPercent = minSpeedPercent;

	// SlideCurveMaxTime: For velocity curve based slide determines how far into the slide curve to start inverse proportional to speed.
	// SlideMaxTime: constant slide speed
	float slideMaxtime = CVar_Feature_SlideMode.GetValueOnGameThread() == 0 ? SlideCurveMaxTime : SlideMaxTime;
	SlideTime = slideMaxtime - (slideMaxtime * speedPercent);

	// Jump displacement is affected by slide (i.e. slide to jump greater distances)
	SlideJumpSpeedMod = SlideJumpSpeedModMax * speedPercent;

	// need to make sure Velocity doesn't affect movement speed during slide otherwise it conflicts with our manual position movement
	Velocity = FVector::ZeroVector;

	// shrink capsul
	CharacterOwner->Crouch();

	OnSlideActionOccured.Broadcast(bIsSliding);

	// TODO: add on screen trail effects
}

void UDeftCharacterMovementComponent::DoImpulse(const FVector& impulseDir)
{
	ImpulseFallDelay = 0.f; // delay our custom falling physics to take over till we actually launch ourselves
	bIsInImpulse = true;
	AddImpulse(impulseDir, true);
}

void UDeftCharacterMovementComponent::StopSlide()
{
	DeftLocks::DecrementInputLockRef();

	SetMovementMode(MOVE_Walking);

	bIsSliding = false;
	SlideTime = 0.f;

	// restore capsul size and shift it down
	CharacterOwner->UnCrouch();

	// restore camera and angle
	OnSlideActionOccured.Broadcast(bIsSliding);

	// TODO: remove on screen trail effects
#if !UE_BUILD_SHIPPING
	Debug_SlideEndPos = CharacterOwner->GetActorLocation();
#endif //!UE_BUILD_SHIPPING
}

void UDeftCharacterMovementComponent::SetCustomFallingMode()
{
	bIsJumping = false;
	bIsFalling = true;
	FallTime = 0.f;
	PrevFallCurveVal = 0.f;
	Velocity.Z = 0.f;				// !important! so that velocity from jump doesn't get carried over to falling

	// hitting something while ascending on the jump is jarring if we use the JumpFall curve which is linear 
	FallCurveToUse = NonJumpFallCurve;
	if (bWasJumpingLastFrame && PrevJumpTime > JumpApexTime)
		FallCurveToUse = JumpFallCurve;

	SetMovementMode(EMovementMode::MOVE_Flying);
}

bool UDeftCharacterMovementComponent::FindFloorBySweep(FFindFloorResult& outFloorResult, const FVector aStartLoc, const FVector aEndLoc)
{
	// More reliable FindFloor() when moving at high speeds
	// Solving paper bullet problem if player is falling very fast (i.e 100+ units in a frame) which means FindFloor would become inaccurate
	// However you can pass a collision test into the FindFloor() function to have it included in the calculation

	FCollisionQueryParams floorCheckCollisionParams;
	floorCheckCollisionParams.AddIgnoredActor(CharacterOwner);
	const UCapsuleComponent* capsulComponent = CharacterOwner->GetCapsuleComponent();
	FCollisionShape capsulShape = FCollisionShape::MakeCapsule(capsulComponent->GetScaledCapsuleRadius(), capsulComponent->GetScaledCapsuleHalfHeight());

	FHitResult floorHitResult;
	const bool bIsBlockingHit = GetWorld()->SweepSingleByProfile(floorHitResult, aStartLoc, aEndLoc, CharacterOwner->GetActorRotation().Quaternion(), capsulComponent->GetCollisionProfileName(), capsulShape, floorCheckCollisionParams);
	if (bIsBlockingHit)
	{
		FindFloor(aStartLoc, outFloorResult, false, &floorHitResult);
		return outFloorResult.IsWalkableFloor() && IsValidLandingSpot(aStartLoc, outFloorResult.HitResult);
	}
	return false;
}

bool UDeftCharacterMovementComponent::FindJumpApexTime(float& outApexTime)
{
	/*
	* Algorithm based off: https://youtu.be/oe2vPXvFLpI?t=821
	* 
		using iterative cross products to find where the normal changes from positive to negative (similar to graphics back face checking) which indicates slope changing from positive to negative (i.e. apex)
		Summary:
			take 3 points (p1, p2, p3) on the curve which are all separated by a step size
				if the normal for (p2-p1) > 0 and (p3-p2) < 0 then the apex is somewhere between p1 & p3
					set new start to p1 and end to p3 and run again with reduced step sizes (halving step size each time)
			
			At any point if our step size leaves the range before finding an apex then we have an invalid graph (i.e. infinitely increasing or decreasing which has no apex)
	*/


	// Time is a constant but needed in Vector form for cross products
	const FVector constTimeXAxis = FVector(1.f, 0.f, 0.f);

	float jumpMin, jumpApexHeight;
	JumpCurve->GetValueRange(jumpMin, jumpApexHeight);

	// Range of where we perform the graph  walking
	float startTime, endTime;
	JumpCurve->GetTimeRange(startTime, endTime);

	// how many times to find the estimated apex 
	// each iteration halves the step size so the more iterations the more accurate but more computationally costly
	int apexIterations = 5;
	float timeStepSize = 0.1f;
	bool isApexFound = false;
	
	// time values at various step sizes (increases as we walk the graph)
	float t1, t2, t3;

	// Each apexIteration narrows the range we step through which gives greater accuracy to the apex detection
	while (apexIterations != 0)
	{
		isApexFound = false;

		t1 = startTime;
		t2 = t1 + timeStepSize;
		t3 = FMath::Min(t2 + timeStepSize, endTime); // making sure the last step is always within our step range 

		if (t2 > endTime || t2 > t3)
		{
			// reached end of curve before finding an apex which means this is an invalid graph
			return false;
		}

		// step through the range defined by startTime and endTime looking for where slope changes from pos to neg
		while (!isApexFound)
		{
			// early out apex checks in case any of the time steps are exactly our apex
			float h1 = JumpCurve->GetFloatValue(t1);
			if (h1 == jumpApexHeight)
			{
				outApexTime = t1;
				return true;
			}

			float h2 = JumpCurve->GetFloatValue(t2);
			if (h2 == jumpApexHeight)
			{
				outApexTime = t2;
				return true;
			}

			float h3 = JumpCurve->GetFloatValue(t3);
			if (h3 == jumpApexHeight)
			{
				outApexTime = t3;
				return true;
			}

			// Compare cross product normals to check if a change in slope (pos -> neg) occurred indicating the apex is somewhere between [t1,t3]
			FVector p1, p2, p3;
			p1 = FVector(t1, h1, 0.f);
			p2 = FVector(t2, h2, 0.f);
			p3 = FVector(t3, h3, 0.f);

			const FVector timeBasisVector = FVector(1.f, 0.f, 0.f);
			FVector normal1 = FVector::CrossProduct(timeBasisVector, p2 - p1);
			FVector normal2 = FVector::CrossProduct(timeBasisVector, p3 - p2);

			if (normal1.Z == 0.f || normal2.Z == 0.f)
			{
				// handles curves which have 0 slope for an extended time
				// curve is parallel to time basis vector. Considering the flat part as apex
				outApexTime = t1;
				return true;
			}

			isApexFound = normal1.Z > 0.f && normal2.Z < 0.f;

			if (!isApexFound)
			{
				t1 += timeStepSize;
				t2 += timeStepSize;
				t3 += timeStepSize;
			}
		}

		--apexIterations;
		startTime = t1;
		endTime = t3;
		timeStepSize /= 2.f;
	}

	// apex time is the midpoint of our range
	outApexTime = (t3 + t1) / 2.f;

	return isApexFound;
}

#if !UE_BUILD_SHIPPING
bool UDeftCharacterMovementComponent::IsJumpCurveEnabled() const
{
	return CVar_FeatureJumpCurve.GetValueOnGameThread() > 0;
}

void UDeftCharacterMovementComponent::DrawDebug()
{
	if (CVar_DebugLocks.GetValueOnGameThread())
		DeftLocks::DrawLockDebug();
	if (CVar_DebugJump.GetValueOnGameThread())
		DrawDebugJump();
	if (CVar_DebugSlide.GetValueOnGameThread())
		DrawDebugSlide();
	if (CVar_DebugFall.GetValueOnGameThread())
		DrawDebugFall();
}

void UDeftCharacterMovementComponent::DrawDebugJump()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::Cyan, TEXT("\n-Jump-"));
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::White, FString::Printf(TEXT("\tJump Apex Height Reached: %f"), Debug_JumpHeightApex));
}

void UDeftCharacterMovementComponent::DrawDebugSlide()
{
	const float slideDistance = (Debug_SlideEndPos - Debug_SlideStartPos).Length();
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::White, TEXT("\n-Slide-"));
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::Cyan, FString::Printf(TEXT("\tDistance %.2f\n\tTime in slide: %.2f\n\tVelocity len: %.2f"), slideDistance, Debug_TimeInSlide, Velocity.Length()));
}

void UDeftCharacterMovementComponent::DrawDebugFall()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.005, bIsFalling ? FColor::Green : FColor::White, FString::Printf(TEXT("\tFalling duration: %.2f"), FallTime));
	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::White, TEXT("\n-Fall-"));
}

#endif//UE_BUILD_SHIPPING
