// Fill out your copyright notice in the Description page of Project Settings.


#include "DeftPlayerCharacter.h"

// Sets default values
ADeftPlayerCharacter::ADeftPlayerCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADeftPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ADeftPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ADeftPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

