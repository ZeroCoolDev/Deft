#include "PredictPathComponent.h"

#include "DeftPlayerCharacter.h"

TAutoConsoleVariable<bool> CVar_DebugPredictPath(TEXT("deft.debug.predictPath"), true, TEXT(""), ECVF_Cheat);


// Sets default values for this component's properties
UPredictPathComponent::UPredictPathComponent()
	: DeftCharacter(nullptr)
	, PredictedPathPoints()
	, PredictedDir(FVector::ZeroVector)
	, PredictedOrigin(FVector::ZeroVector)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UPredictPathComponent::BeginPlay()
{
	Super::BeginPlay();

	DeftCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (!DeftCharacter.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftCharacter from owner!"));
		return;
	}
}


// Called every frame
void UPredictPathComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	if (CVar_DebugPredictPath.GetValueOnGameThread())
		DebugDraw();
#endif //!UE_BUILD_SHIPPING
}

bool UPredictPathComponent::PredictPath_Parabola(float aSpeed, float aAngle, const FVector& aDir, const FVector& aPathEnd, TArray<FVector>& outPath)
{
	// Velocity is in the actors forward with a local pitch rotated by an aAngle 
	FVector localRight = DeftCharacter->GetActorRightVector();
	FVector velocity = DeftCharacter->GetActorForwardVector().GetSafeNormal();
	velocity = velocity.RotateAngleAxis(aAngle, -localRight) * aSpeed; // for some reason we have to give the 'left' axis to get correct CW positive rotation

#if !UE_BUILD_SHIPPING
	PredictedOrigin = DeftCharacter->GetActorLocation();
	PredictedDir = aDir; //TODO: we need to rotate to face aDir because that's actually where the grapple was shot
	PredictedEnd = aPathEnd;
	FVector localUp = DeftCharacter->GetActorUpVector();
	float angleCos = velocity.Dot(localUp);
	float angleDeg = 90.f - FMath::RadiansToDegrees(FMath::Acos(angleCos));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::Yellow, FString::Printf(TEXT("angle between XY plane and vector: %.2f degrees"), angleDeg));
#endif//!UE_BUILD_SHIPPING


	// find the total horizontal displacement
	const float gravity = -980.f; //TODO: either take in, or read from WorldSettings on ctor

	//TODO: We can use trig for 2/3 of the axes but unsure which ones.
	// Obviously projecting works but is a little less efficient than just simple trig multiplication

	// vertical velocity
	//float vY = aSpeed * FMath::Sin(rads);
	FVector zBasis = DeftCharacter->GetActorUpVector();
	const FVector projZ = (velocity.Dot(zBasis) / zBasis.Dot(zBasis)) * zBasis;
	const float velocityZ = projZ.Length();
	float deltaVZ = -(velocityZ * 2);

	// time
	float deltaT = deltaVZ / gravity;

	// horizontal velocity
	//float vX = aSpeed * FMath::Cos(rads);
	FVector xBasis = DeftCharacter->GetActorForwardVector();
	const FVector projX = (velocity.Dot(xBasis) / xBasis.Dot(xBasis)) * xBasis;
	const float velocityX = projX.Length();
	// displacement
	float displacement = velocityX * deltaT;

	// y velocity
	FVector yBasis = DeftCharacter->GetActorRightVector();
	const FVector projY = (velocity.Dot(yBasis) / yBasis.Dot(yBasis)) * yBasis;
	const float velocityY = projY.Length();

#if !UE_BUILD_SHIPPING
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::Yellow, FString::Printf(TEXT("horizontal displacement: %.2f\nair time: %.2f"), displacement, deltaT));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::Yellow, FString::Printf(TEXT("Velocity in x: %.2f, y: %.2f, z: %.2f"), velocityX, velocityY, velocityZ));
	DrawDebugLine(GetWorld(), PredictedOrigin, PredictedOrigin + (DeftCharacter->GetActorForwardVector() * displacement), FColor::Cyan);
#endif//UE_BUILD_SHIPPING

	// plot the trajectory
	const float stepSize = 0.05f;
	float time = 0.f;

	outPath.Empty();
	
	const FVector pathEnd2D = aPathEnd * FVector(1.f, 1.f, 0.f);
	const FVector origin2D = PredictedOrigin * FVector(1.f, 1.f, 0.f);
	// Used to compare with each point since since will be past our attachment point which we can ignore
	const float distToEnd2D = FMath::Abs((aPathEnd - origin2D).Length());

	while (time <= deltaT)
	{
		const float xPos = velocityX * time;
		const float yPos = velocityY * time;
		// NOTICE: gravity needs to be positive in this formula (for whatever reason...)
		const float zPos = (velocityZ * time) - ((-gravity * (time * time)) / 2.f);
		FVector pos = FVector(xPos, yPos, zPos);
		// this ^ is the local agents position forward, so make sure we rotate it however the agent is rotated.
		// TODO: we actually need to rotate again by the difference between agent's rotation and where the grapple actually hit
		const FRotator actorRotation = DeftCharacter->GetActorRotation();
		pos = pos.RotateAngleAxis(actorRotation.Pitch, -FVector::RightVector);
		pos = pos.RotateAngleAxis(actorRotation.Yaw, FVector::UpVector);
		pos = pos.RotateAngleAxis(actorRotation.Roll, -FVector::ForwardVector);
		// Finally we need to translate the path to originate at our player
		pos += PredictedOrigin;

		// need to start measuring from the origin versus 0,0,0
		const FVector pos2D = (pos) * FVector(1.f, 1.f, 0.f);
		const float distToPos2D = FMath::Abs((pos2D - origin2D).Length());

		// if the point is not between the player and the final point disregard it.
		const FVector endToPoint = pos - aPathEnd;
		const FVector playerToEnd = aPathEnd - PredictedOrigin;
		const float dot = playerToEnd.GetSafeNormal().Dot(endToPoint.GetSafeNormal());

		// Only add the path up until the end
		if (dot >= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("adding pos: %s"), *pos.ToString());
			outPath.Add(pos);
		}
		else // the moment we go further than the end we can stop
		{
			UE_LOG(LogTemp, Error, TEXT("Ignoring point %s because it's %.2d, past the end"), *pos.ToString(), dot);
		}
			//break;


		time += stepSize;
	}

#if !UE_BUILD_SHIPPING
	PredictedPathPoints = outPath;
#endif//UE_BUILD_SHIPPING

	return false;
}

#if !UE_BUILD_SHIPPING
void UPredictPathComponent::DebugDraw()
{
	if (!DeftCharacter.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("DeftCharacter is not valid, aborting draw"));
		return;
	}

	const FVector origin = PredictedOrigin;

	// world axes
	DrawDebugLine(GetWorld(), origin, origin + (FVector::ForwardVector * 50.f), FColor::Red, false, -1.f, 0u, 2.f);
	DrawDebugLine(GetWorld(), origin, origin + (FVector::RightVector * 50.f), FColor::Green, false, -1.f, 0u, 2.f);
	DrawDebugLine(GetWorld(), origin, origin + (FVector::UpVector * 50.f), FColor::Blue, false, -1.f, 0u, 2.f);

	// local axes
	DrawDebugLine(GetWorld(), origin, origin + (DeftCharacter->GetActorForwardVector() * 70.f), FColor::Magenta);
	DrawDebugLine(GetWorld(), origin, origin + (DeftCharacter->GetActorRightVector() * 70.f), FColor::Emerald);
	DrawDebugLine(GetWorld(), origin, origin + (DeftCharacter->GetActorUpVector() * 70.f), FColor::Cyan);

	DrawDebugLine(GetWorld(), origin, origin + (PredictedEnd - origin), FColor::Yellow);

	DrawDebugSphere(GetWorld(), origin, 5.f, 12, FColor::White);

	if (PredictedPathPoints.IsEmpty())
		return;

	// Draw predicted trajectory
	FVector prevPoint = origin;
	for (int i = 0, end = PredictedPathPoints.Num(); i < end; ++i)
	{
		DrawDebugLine(GetWorld(), prevPoint, PredictedPathPoints[i], FColor::Green);
		prevPoint = PredictedPathPoints[i];
		DrawDebugSphere(GetWorld(), PredictedPathPoints[i], 5.f, 12.f, i == 0 ? FColor::Green : i == end - 1 ? FColor::Purple : FColor::Yellow);
	}
}
#endif//UE_BUILD_SHIPPING
