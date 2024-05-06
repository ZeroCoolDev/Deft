// Fill out your copyright notice in the Description page of Project Settings.


#include "TrajectoryComponent.h"

// Sets default values for this component's properties
UTrajectoryComponent::UTrajectoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	Owner = GetOwner();
}

void UTrajectoryComponent::Init(float aSpeed, float aAngle)
{
	// given an angle we can get a unit vector in that direction
	// multiply by speed to get V
	// then find the time in air and displacement.


}

// Called when the game starts
void UTrajectoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UTrajectoryComponent::CalculateTrajectory()
{

}

// Called every frame
void UTrajectoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

