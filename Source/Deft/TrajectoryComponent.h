// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TrajectoryComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UTrajectoryComponent : public UActorComponent
{
	GENERATED_BODY()

private:
	struct RotMatrix
	{
		RotMatrix(float aDegrees)
			: Degrees(aDegrees)
		{
			Cos = FMath::Cos(FMath::DegreesToRadians(Degrees));
			Sin = FMath::Sin(FMath::DegreesToRadians(Degrees));
		}

		void AroundX(float(*outMatrix)[3][3]);
		void AroundY(float(*outMatrix)[3][3]);
		void AroundZ(float(*outMatrix)[3][3]);
	private:
		float Degrees;
		float Cos;
		float Sin;
	};

public:	
	// Sets default values for this component's properties
	UTrajectoryComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void Init();
	void Init(float aSpeed, float aAngle);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Trajectory)
	bool AroundX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Trajectory)
	bool AroundY;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Trajectory)
	bool AroundZ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Trajectory)
	float TestSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Trajectory)
	float TestAngle;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void CalculateTrajectory(const FVector& Axis, float aAngle, const FVector& V);
	AActor* Owner;

private:
	void Draw();

	FVector Velocity;
	float Angle;
};
