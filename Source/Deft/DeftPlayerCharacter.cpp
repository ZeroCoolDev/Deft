#include "DeftPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ADeftPlayerCharacter::ADeftPlayerCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArmComp"));
	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));

	// Set the location and rotation of the character mesh transform
	if (USkeletalMeshComponent* skeletalMesh = GetMesh())
	{
		skeletalMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FQuat(FRotator(0.f, -90.f, 0.f)));

		// attach your class components to the default characters skeletal mesh component
		SpringArmComp->SetupAttachment(skeletalMesh);
	}

	CameraComp->SetupAttachment(SpringArmComp, USpringArmComponent::SocketName);
	CameraComp->bUsePawnControlRotation = true;
}

// Called when the game starts or when spawned
void ADeftPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* playerController = static_cast<APlayerController*>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* inputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(playerController->GetLocalPlayer()))
		{
			inputSubsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ADeftPlayerCharacter::Move(const FInputActionValue& aValue)
{
	// input is a vector2S
	FVector2D input = aValue.Get<FVector2D>();
	UE_LOG(LogTemp, Warning, TEXT("Move Fwd/Back: %.2f, Right/Left: %.2f"), input.Y, input.X);

	// TODO: add custom player controller
	if (Controller)
	{
		// find out which way is forward
		const FRotator rotation = Controller->GetControlRotation();
		const FRotator yawRotation(0, rotation.Yaw, 0);

		// get forward vector
		// TODO: is there a benefit from getting forward form the rotation and not just ActorForwardVector?
		const FVector forwardDir = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
		const FVector rightDir = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(forwardDir, input.Y);
		AddMovementInput(rightDir, input.X);

		UE_LOG(LogTemp, Warning, TEXT("speed: %.2f"), GetCharacterMovement()->Velocity.Size());
	}
}

void ADeftPlayerCharacter::Look(const FInputActionValue& aValue)
{
	// input is a vector2S
	FVector2D input = aValue.Get<FVector2D>();
	//UE_LOG(LogTemp, Warning, TEXT("Look Up/Down: %.2f, Right/Left: %.2f"), input.Y, input.X);

	if (Controller)
	{
		AddControllerYawInput(input.X);
		AddControllerPitchInput(input.Y);
	}
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

	if (UEnhancedInputComponent* inputComp = static_cast<UEnhancedInputComponent*>(PlayerInputComponent))
	{
		// Jump - TODO: perhaps write my own jump functions
		inputComp->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ADeftPlayerCharacter::Jump);
		inputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADeftPlayerCharacter::StopJumping);

		// Move
		inputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADeftPlayerCharacter::Move);

		// Look
		inputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADeftPlayerCharacter::Look);
	}
}

