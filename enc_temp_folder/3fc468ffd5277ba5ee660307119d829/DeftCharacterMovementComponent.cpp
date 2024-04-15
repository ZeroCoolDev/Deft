#include "DeftCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

TAutoConsoleVariable<bool> CVar_UseEngineJump(TEXT("deft.jump.UseJumpCurve"), true, TEXT("true=use custom jump curve logic, false=use engine jump logic"));

UDeftCharacterMovementComponent::UDeftCharacterMovementComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UDeftCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	bIsJumpFalling = false;

	if (JumpCurve)
	{
		JumpCurve->GetTimeRange(JumpCurveMinTime, JumpCurveMaxTime);

		float minHeight;
		JumpCurve->GetValueRange(minHeight, JumpApexHeight);
		
		bIsValidJumpCurve = FindJumpApexTime(JumpApexTime);
		if (!bIsValidJumpCurve)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid Jump Curve, no Apex found"));
		}

		UE_LOG(LogTemp, Warning, TEXT("Jump Curve Time (min,max): (%f, %f)"), JumpCurveMinTime, JumpCurveMaxTime);
		UE_LOG(LogTemp, Warning, TEXT("Jump Apex Height %f at time %f"), JumpApexTime, JumpApexHeight);
	}
}

void UDeftCharacterMovementComponent::TickComponent(float aDeltaTime, enum ELevelTick aTickType, FActorComponentTickFunction* aThisTickFunction)
{
	Super::TickComponent(aDeltaTime, aTickType, aThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
	{
		return;
	}
#endif
	
	ProcessJump(aDeltaTime);
	ProcessJumpFall(aDeltaTime);
}

// TODO: there is a bug where you can just hold the space bar and jump, so we don't want to reset the jump if the jump button is literally down
bool UDeftCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("DoJump::Jump curve disabled"));
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
			if (bIsValidJumpCurve && JumpCurve)
			{
				// Ignore gravity
				SetMovementMode(MOVE_Flying);

				// Starting jump
				bIsJumping = true;
				bIsJumpFalling = false;
				JumpTime = JumpCurveMinTime;
				PrevJumpTime = JumpTime;
				PrevJumpCurveVal = JumpCurve->GetFloatValue(JumpTime);

#if !UE_BUILD_SHIPPING
				JumpHeightApexTest = 0.f;
#endif//!UE_BUILD_SHIPPING

				return true;
			}
		}
	};

	return false;
}

bool UDeftCharacterMovementComponent::IsFalling() const
{
#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
		return Super::IsFalling();
#endif

	if (bIsValidJumpCurve && JumpCurve)
		return Super::IsFalling() || bIsJumping || bIsJumpFalling;

	return false;
}

void UDeftCharacterMovementComponent::ProcessJump(float aDeltaTime)
{
	if (!bIsValidJumpCurve)
		return;

	if (!JumpCurve)
		return;

	if (!bIsJumping)
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
		// TODO: consider using custom and making specific movement modes for these custom jump & falling
		// which means we'll have to handle input when the character is in the air just as a heads up
		Velocity.Z = 0.f;
		float yVelocity = jumpCurveValDelta / aDeltaTime; //note: velocity = distance / time

		const FVector actorLocation = GetActorLocation();
		FVector destinationLocation = actorLocation + FVector(0.f, 0.f, jumpCurveValDelta);

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
				CharacterOwner->ResetJumpState();

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

		// In jump but descending
		if (yVelocity < 0.f)
		{
			// There is no floor checks while in MOVE_Flying so do it manually
			const FVector capsulLoc = UpdatedComponent->GetComponentLocation();
			FFindFloorResult floorResult;
			if (CustomFindFloor(floorResult, capsulLoc, destinationLocation))
			{
				const float floorDistance = floorResult.GetDistanceToFloor();
				if (FMath::Abs(jumpCurveValDelta) > floorDistance)
				{
					destinationLocation = capsulLoc - FVector(0.f, 0.f, jumpCurveValDelta);
				}

				SetMovementMode(MOVE_Walking);

				bIsJumping = false;
				CharacterOwner->ResetJumpState();
			}
		}

		// TODO: can the MoveComponentTo below go in an else here? I feel like if we hit something and are now falling we don't want to move the component anywhere
		// Move the actual capsule component in the world
		FLatentActionInfo latentInfo;
		latentInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLocation, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Type::Move, latentInfo);

		if (isJumpApexReached)
		{
			if (bNotifyApex)
			{
				NotifyJumpApex();
			}
		}

	}
	else
	{
		// reached the end of the jump curve

#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("Jump Apex Height Reached: %f"), JumpHeightApexTest);
#endif //!UE_BUILD_SHIPPING

		// check for a floor
		const FVector capsulLoc = UpdatedComponent->GetComponentLocation();
		FFindFloorResult floorResult;
		FindFloor(capsulLoc, floorResult, false); // TODO: implement this myself using collision checks?
		if (floorResult.IsWalkableFloor() && IsValidLandingSpot(capsulLoc, floorResult.HitResult))
			SetMovementMode(MOVE_Walking);
		else
			SetCustomFallingMode();

		bIsJumping = false;
		CharacterOwner->ResetJumpState();
	}
}

void UDeftCharacterMovementComponent::ProcessJumpFall(float aDeltaTime)
{
	if (!JumpFallCurve)
		return;

	if (bIsJumpFalling)
	{
		JumpFallTime += aDeltaTime;

		const float jumpfallCurveVal = JumpFallCurve->GetFloatValue(JumpFallTime);
		UE_LOG(LogTemp, Warning, TEXT("fall curve val %f at time %f"), jumpfallCurveVal, JumpFallTime);
		const float deltaJumpfallCurveVal = jumpfallCurveVal - PrevJumpFallCurveVal;
		PrevJumpFallCurveVal = jumpfallCurveVal;

		Velocity.Z = 0.f;

		const FVector capsuleLocation = UpdatedComponent->GetComponentLocation();
		FVector destinationLocation = capsuleLocation + FVector(0.f, 0.f, deltaJumpfallCurveVal);

		FFindFloorResult floorResult;
		if (CustomFindFloor(floorResult, capsuleLocation, destinationLocation))
		{
			const float floorDistance = floorResult.GetDistanceToFloor(); //TODO: could do this manually by checking the distance from the actorLocation to the hit result
			if (FMath::Abs(deltaJumpfallCurveVal) > floorDistance)
			{
				destinationLocation = capsuleLocation - FVector(0.f, 0.f, floorDistance);
			}

			bIsJumpFalling = false;

			// Stopping the character and canceling all the movement carried from before the jump/fall
			// note: Remove if you want to carry the momentum
			Velocity = FVector::ZeroVector;

			SetMovementMode(MOVE_Walking);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("falling, floor NOT found"));
		}

		FLatentActionInfo latentInfo;
		latentInfo.CallbackTarget = this;
		UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLocation, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Move, latentInfo);
	}
	else if (MovementMode == EMovementMode::MOVE_Falling)
	{
		// Dropping down from ledge while walking should use our custom fall logic

		// note: not doing it during engine jump since engine jump uses falling
#if !UE_BUILD_SHIPPING
		if (!IsJumpCurveEnabled())
			return;
#endif

		SetCustomFallingMode();
	}
}

void UDeftCharacterMovementComponent::SetCustomFallingMode()
{
	if (JumpFallCurve)
	{
		UE_LOG(LogTemp, Warning, TEXT("processing jump fall")); // maybe we need to set bIsJumping to false?

		bIsJumpFalling = true;
		JumpFallTime = 0.f;
		PrevJumpFallCurveVal = 0.f;
		Velocity.Z = 0.f;				// !important! so that velocity from jump doesn't get carried over to falling

		SetMovementMode(EMovementMode::MOVE_Flying);
	}
	else
		SetMovementMode(MOVE_Falling);
}

bool UDeftCharacterMovementComponent::CustomFindFloor(FFindFloorResult& outFloorResult, const FVector aStartLoc, const FVector aEndLoc)
{
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
				outApexTime = t1; // TODO: check I'm using t1 but he was using t2
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
