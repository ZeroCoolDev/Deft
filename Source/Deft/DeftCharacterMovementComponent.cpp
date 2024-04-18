#include "DeftCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "DeftPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"

TAutoConsoleVariable<bool> CVar_UseEngineJump(TEXT("deft.jump.UseJumpCurve"), true, TEXT("true=use custom jump curve logic, false=use engine jump logic"));
TAutoConsoleVariable<float> CVar_SetLoadPercent(TEXT("deft.slide.SetSlideStart"), 0.f, TEXT("set where in the slide curve slides should start"));

UDeftCharacterMovementComponent::UDeftCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, JumpCurve(nullptr)
	, FallCurve(nullptr)
	, SlideDirection(FVector::ZeroVector)
	, SlideJumpAdditive(FVector::ZeroVector)
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
	, SlideCurveStartTime(0.f)
	, SlideCurveMaxTime(0.f)
	, SlideMinimumStartTime(0.f)
	, SlideJumpSpeedMod(0.f)
	, SlideJumpSpeedModMax(0.f)
	, bIsJumping(false)
	, bIsValidJumpCurve(false)
	, bIsFalling(false)
	, bIsSliding(false)
{
}

void UDeftCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

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


	if (!FallCurve)
		UE_LOG(LogTemp, Error, TEXT("Missing Fall Curve"));

	if (SlideCurve)
	{
		SlideCurve->GetTimeRange(SlideCurveStartTime, SlideCurveMaxTime);
		UE_LOG(LogTemp, Warning, TEXT("Slide Curve Time (min,max): (%f, %f)"), SlideCurveStartTime, SlideCurveMaxTime);
	}
	else 
		UE_LOG(LogTemp, Error, TEXT("Missing Slide Curve"));

	SlideMinimumStartTime = 0.3;
	SlideJumpSpeedModMax = 4.f;
}

void UDeftCharacterMovementComponent::TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction)
{
	Super::TickComponent(aDeltaTime, aTickType, aThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
		return;
#endif //!UE_BUILD_SHIPPING
	
	ProcessJumping(aDeltaTime);
	ProcessFalling(aDeltaTime);
	ProcessSliding(aDeltaTime);
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
			JumpHeightApexTest = 0.f;
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
	return Super::IsFalling() || bIsJumping || bIsFalling;
}

bool UDeftCharacterMovementComponent::CanAttemptJump() const
{
	return IsJumpAllowed() &&
		!bWantsToCrouch &&
		(IsMovingOnGround() || IsFalling() || bIsSliding);
}

void UDeftCharacterMovementComponent::ProcessJumping(float aDeltaTime)
{
	if (!JumpCurve || !bIsJumping)
		return;

	JumpTime += aDeltaTime;
	if (JumpTime <= JumpCurveMaxTime)
	{
		// Get the new character location
		float jumpCurveVal = JumpCurve->GetFloatValue(JumpTime);

		// Make sure that the character always reaches the jump apex height and notify character about reaching apex
		bool isJumpApexReached = PrevJumpTime < JumpApexTime&& JumpTime > JumpApexTime;
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

#if !UE_BUILD_SHIPPING
			// accumulating distance traveled UPWARDS to find the apex
			JumpHeightApexTest += (destinationLocation.Z - actorLocation.Z);
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
		{
			OnLandedFromAir.Broadcast();
#if !UE_BUILD_SHIPPING
			UE_LOG(LogTemp, Warning, TEXT("Jump Apex Height Reached: %f"), JumpHeightApexTest);
#endif //!UE_BUILD_SHIPPING
		}
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
		}
		else
			SetCustomFallingMode();


		CharacterOwner->StopJumping();

#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Jump Apex Height Reached: %f"), JumpHeightApexTest);
#endif //!UE_BUILD_SHIPPING
	}
}

void UDeftCharacterMovementComponent::ProcessFalling(float aDeltaTime)
{
	if (!FallCurve)
		return;

	if (bIsFalling)
	{
		FallTime += aDeltaTime;

		const float fallCurveVal = FallCurve->GetFloatValue(FallTime);
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

	// move component based off curve (increasing speed essentially)
	SlideTime += aDeltaTime;
	if (SlideTime > SlideCurveMaxTime)
	{
		StopSlide();
		return;
	}

	const float slideCurveVal = SlideCurve->GetFloatValue(SlideTime);
	const FVector capsuleLocation = UpdatedComponent->GetComponentLocation();
	const FVector destinationLocation = capsuleLocation + (SlideDirection * slideCurveVal);

	FLatentActionInfo latentInfo;
	latentInfo.CallbackTarget = this;
	UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLocation, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);
}

void UDeftCharacterMovementComponent::DoSlide()
{
	// Only allow sliding while on the ground
	if (!IsMovingOnGround())
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot slide because not moving on ground"));
		return;
	}

	SetMovementMode(MOVE_Flying);

	bIsSliding = true;

	// slide in the direction of the player's last velocity but independent of it's speed (which is constant during slide excluding jump)
	SlideDirection = Velocity.GetSafeNormal();

	// min speed percent makes sure we can still slide even at very low velocities
	const float minSpeedPercent = 0.5f;
	const float speedPercent = FMath::Max(minSpeedPercent , Velocity.Length() / GetMaxSpeed());

	// Determines how far into the slide curve to start inverse proportional to speed, meaning:
	// Max Speed = Max Slide length, 50% speed = 50% slide length. slower speeds when entering slide results in shorter slide duration
	SlideTime = SlideCurveMaxTime - (SlideCurveMaxTime * speedPercent);

	// Jump displacement is affected by slide (i.e. slide to jump greater distances)
	SlideJumpSpeedMod = SlideJumpSpeedModMax * speedPercent;

	// set velocity to max speed so that the slide speed is constant 
	Velocity = (Velocity.GetSafeNormal() * GetMaxSpeed());


	// shrink capsul
	// lower camera and angle left
	// lock WASD movement
	// TODO: add on screen trail effects
}

void UDeftCharacterMovementComponent::StopSlide()
{
	UE_LOG(LogTemp, Log, TEXT("StopSlide!"));

	if (ADeftPlayerCharacter* deftPlayerCharacter = Cast<ADeftPlayerCharacter>(CharacterOwner))
		deftPlayerCharacter->StopSlide();

	SetMovementMode(MOVE_Walking);

	bIsSliding = false;
	SlideTime = 0.f;
	// restore capsul size
	// restore camera and angle
	// un-lock WASD movement
	// TODO: remove on screen trail effects
}

void UDeftCharacterMovementComponent::SetCustomFallingMode()
{
	bIsJumping = false;
	bIsFalling = true;
	FallTime = 0.f;
	PrevFallCurveVal = 0.f;
	Velocity.Z = 0.f;				// !important! so that velocity from jump doesn't get carried over to falling

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
	return CVar_UseEngineJump.GetValueOnGameThread();
}
#endif//UE_BUILD_SHIPPING
