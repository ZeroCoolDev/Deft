#include "DeftCharacterMovementComponent.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

TAutoConsoleVariable<bool> CVar_UseEngineJump(TEXT("deft.jump.UseJumpCurve"), true, TEXT("true=use custom jump curve logic, false=use engine jump logic"));

void UDeftCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	if (JumpCurve)
	{
		JumpCurve->GetTimeRange(JumpCurveMinTime, JumpCurveMaxTime);
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

	if (bIsJumping)
	{
		JumpTime += aDeltaTime;

		if (JumpTime <= JumpCurveMaxTime)
		{
			// Get the new character location
			const float jumpCurveVal = JumpCurve->GetFloatValue(JumpTime);
			const float jumpCurveValDelta = jumpCurveVal - PrevJumpCurveVal;
			PrevJumpCurveVal = jumpCurveVal;

			Velocity.Z = jumpCurveValDelta / aDeltaTime; //note: velocity = distance / time

			const FVector actorLoc = GetActorLocation();
			FVector destinationLoc = actorLoc + FVector(0.f, 0.f, jumpCurveValDelta);

			// Check roof collision if character is moving up
			if (Velocity.Z > 0.f)
			{
				FCollisionQueryParams roofCheckCollisionParams;
				roofCheckCollisionParams.AddIgnoredActor(CharacterOwner);
				// TODO: could cache this since the capsule shape isn't going to change dynamically 
				const UCapsuleComponent* capsulComponent = CharacterOwner->GetCapsuleComponent();
				FCollisionShape capsulShape = FCollisionShape::MakeCapsule(capsulComponent->GetScaledCapsuleRadius(), capsulComponent->GetScaledCapsuleHalfHeight());

				FHitResult roofHitResult;
				const bool bIsBlockingHit = GetWorld()->SweepSingleByProfile(roofHitResult, actorLoc, destinationLoc, CharacterOwner->GetActorRotation().Quaternion(), capsulComponent->GetCollisionProfileName(), capsulShape, roofCheckCollisionParams);
				if (bIsBlockingHit)
				{
					// Roof collision hit
					bIsJumping = false;
					CharacterOwner->ResetJumpState();

					// Reset vertical velocity to let gravity do the work
					Velocity.Z = 0.f;

					// Take character to a safe location where its not hitting roof
					// TODO: this can be improved by using the impact location and using that
					destinationLoc = actorLoc;

					SetMovementMode(EMovementMode::MOVE_Falling);
				}
			}
			// TODO: can the MoveComponentTo below go in an else here? I feel like if we hit something and are now falling we don't want to move the component anywhere
			// Move the actual capsule component in the world
			FLatentActionInfo latentInfo;
			latentInfo.CallbackTarget = this;
			UKismetSystemLibrary::MoveComponentTo((USceneComponent*)CharacterOwner->GetCapsuleComponent(), destinationLoc, CharacterOwner->GetActorRotation(), false, false, 0.f, true, EMoveComponentAction::Type::Move, latentInfo);
			
			if (Velocity.Z < 0.f)
			{
				// There is no floor checks while in MOVE_Flying so do it manually
				const FVector capsulLoc = UpdatedComponent->GetComponentLocation();
				FFindFloorResult floorResult;
				FindFloor(capsulLoc, floorResult, false); // TODO: implement this myself using collision checks?
				if (floorResult.IsWalkableFloor() && IsValidLandingSpot(capsulLoc, floorResult.HitResult))
				{
					SetMovementMode(MOVE_Walking);

					bIsJumping = false;
					CharacterOwner->ResetJumpState();
				}
			}
		
		}
		else
		{
			// reached the end of the jump curve

			// check for a floor
			const FVector capsulLoc = UpdatedComponent->GetComponentLocation();
			FFindFloorResult floorResult;
			FindFloor(capsulLoc, floorResult, false); // TODO: implement this myself using collision checks?
			if (floorResult.IsWalkableFloor() && IsValidLandingSpot(capsulLoc, floorResult.HitResult))
				SetMovementMode(MOVE_Walking);
			else
				SetMovementMode(MOVE_Falling);

			bIsJumping = false;
			CharacterOwner->ResetJumpState();
		}
	}
}

bool UDeftCharacterMovementComponent::DoJump(bool bReplayingMoves)
{
#if !UE_BUILD_SHIPPING
	if (!IsJumpCurveEnabled())
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.f, FColor::Red, TEXT("DoJump::Jump curve disabled"));
		return Super::DoJump(bReplayingMoves);
	}
#endif

	if (bIsJumping)
		return false;

	if (CharacterOwner && CharacterOwner->CanJump()) // TODO: 'CanJump()' may have to be overridden if we wanna allow double jump stuff
	{
		// Don't jump if we can't move up/down
		if (!bConstrainToPlane || FMath::Abs(PlaneConstraintNormal.Z) != 1.f)
		{
			if (JumpCurve)
			{
				// Ignore gravity
				SetMovementMode(MOVE_Flying);

				// Starting jump
				bIsJumping = true;
				JumpTime = JumpCurveMinTime;
				PrevJumpCurveVal = JumpCurve->GetFloatValue(JumpTime);

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

	if (JumpCurve)
		return Super::IsFalling() || bIsJumping;

	return false;
}

#if !UE_BUILD_SHIPPING
bool UDeftCharacterMovementComponent::IsJumpCurveEnabled() const
{
	return CVar_UseEngineJump.GetValueOnGameThread();
}
#endif//UE_BUILD_SHIPPING
