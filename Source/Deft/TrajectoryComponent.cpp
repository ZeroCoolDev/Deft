// Fill out your copyright notice in the Description page of Project Settings.


#include "TrajectoryComponent.h"

// Sets default values for this component's properties
UTrajectoryComponent::UTrajectoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

void UTrajectoryComponent::Init(float aSpeed, float aAngle)
{
	FVector v = FVector::ForwardVector;

	// Only un-comment for an engine sanity check
	//CalculateTrajectory(AroundX ? FVector::ForwardVector : AroundY ? FVector::RightVector : FVector::UpVector, FMath::DegreesToRadians(aAngle), v);

	// 0, 1, 0
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::Yellow, FString::Printf(TEXT("%s"), *v.ToString()));

	float cos = FMath::Cos(FMath::DegreesToRadians(aAngle));
	float sin = FMath::Sin(FMath::DegreesToRadians(aAngle));

	RotMatrix rotMatrixDef(aAngle);
	float rotMatrix[3][3];

	if (AroundX)
		rotMatrixDef.AroundX(&rotMatrix);
	if (AroundY)
		rotMatrixDef.AroundY(&rotMatrix);
	if (AroundZ)
		rotMatrixDef.AroundZ(&rotMatrix);

	float x = (v.X * rotMatrix[0][0]) + (v.Y * rotMatrix[1][0]) + (v.Z * rotMatrix[2][0]);
	float y = (v.X * rotMatrix[0][1]) + (v.Y * rotMatrix[1][1]) + (v.Z * rotMatrix[2][1]);
	float z = (v.X * rotMatrix[0][2]) + (v.Y * rotMatrix[1][2]) + (v.Z * rotMatrix[2][2]);

	FVector direction = FVector(x, y, z);
	Velocity = direction * aSpeed;

	Angle = aAngle;
}

void UTrajectoryComponent::Init()
{
	Init(TestSpeed, TestAngle);
}

// Called when the game starts
void UTrajectoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Owner = GetOwner();
}


// taken from FVector::RotateAngleAxis just to compare and confirm we're rotating the correct pos/neg directions
// always rotate positively CCW
void UTrajectoryComponent::CalculateTrajectory(const FVector& Axis, float aAngle, const FVector& V)
{
	float S, C;
	FMath::SinCos(&S, &C, aAngle);

	const float XX = Axis.X * Axis.X;
	const float YY = Axis.Y * Axis.Y;
	const float ZZ = Axis.Z * Axis.Z;

	const float XY = Axis.X * Axis.Y;
	const float YZ = Axis.Y * Axis.Z;
	const float ZX = Axis.Z * Axis.X;

	const float XS = Axis.X * S;
	const float YS = Axis.Y * S;
	const float ZS = Axis.Z * S;

	const float OMC = 1.f - C;

	FVector rotated = FVector(
		(OMC * XX + C) * V.X + (OMC * XY - ZS) * V.Y + (OMC * ZX + YS) * V.Z,
		(OMC * XY + ZS) * V.X + (OMC * YY + C) * V.Y + (OMC * YZ - XS) * V.Z,
		(OMC * ZX - YS) * V.X + (OMC * YZ + XS) * V.Y + (OMC * ZZ + C) * V.Z
		);

	const FVector origin = Owner->GetActorLocation();
	DrawDebugLine(GetWorld(), origin, origin + (rotated * 200.f), FColor::Magenta);
}

void UTrajectoryComponent::Draw()
{
	const FVector origin = Owner->GetActorLocation();

	DrawDebugLine(GetWorld(), origin, origin + (FVector::ForwardVector * 50.f), FColor::Red);
	DrawDebugLine(GetWorld(), origin, origin + (FVector::RightVector * 50.f), FColor::Green);
	DrawDebugLine(GetWorld(), origin, origin + (FVector::UpVector * 50.f), FColor::Blue);

	const FVector dest = Velocity;
	DrawDebugLine(GetWorld(), origin, origin + dest, FColor::Yellow);

	DrawDebugSphere(GetWorld(), origin, 25.f, 12, FColor::White);
}

// Called every frame
void UTrajectoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Init();
	Draw();
}

void UTrajectoryComponent::RotMatrix::AroundX(float(*outMatrix)[3][3])
{
	// X axis 
	(*outMatrix)[0][0] = 1;
	(*outMatrix)[0][1] = 0;
	(*outMatrix)[0][2] = 0;

	// Y axis 
	(*outMatrix)[1][0] = 0;
	(*outMatrix)[1][1] = Cos;
	(*outMatrix)[1][2] = Sin;

	// z axis 
	(*outMatrix)[2][0] = 0;
	(*outMatrix)[2][1] = -Sin;
	(*outMatrix)[2][2] = Cos;
}

void UTrajectoryComponent::RotMatrix::AroundY(float(*outMatrix)[3][3])
{
	// X axis 
	(*outMatrix)[0][0] = Cos;
	(*outMatrix)[0][1] = 0;
	(*outMatrix)[0][2] = -Sin;

	// Y axis 
	(*outMatrix)[1][0] = 0;
	(*outMatrix)[1][1] = 1;
	(*outMatrix)[1][2] = 0;

	// z axis 
	(*outMatrix)[2][0] = Sin;
	(*outMatrix)[2][1] = 0;
	(*outMatrix)[2][2] = Cos;
}

void UTrajectoryComponent::RotMatrix::AroundZ(float(*outMatrix)[3][3])
{
	// X axis 
	(*outMatrix)[0][0] = Cos;
	(*outMatrix)[0][1] = Sin;
	(*outMatrix)[0][2] = 0;

	// Y axis 
	(*outMatrix)[1][0] = -Sin;
	(*outMatrix)[1][1] = Cos;
	(*outMatrix)[1][2] = 0;

	// z axis 
	(*outMatrix)[2][0] = 0;
	(*outMatrix)[2][1] = 0;
	(*outMatrix)[2][2] = 1;
}
