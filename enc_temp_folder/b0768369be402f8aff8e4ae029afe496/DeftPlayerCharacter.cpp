#include "DeftPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "CameraMovementComponent.h"
#include "DeftCharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
ADeftPlayerCharacter::ADeftPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDeftCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
	, SpringArmComp(nullptr)
	, CameraComp(nullptr)
	, DefaultMappingContext(nullptr)
	, JumpAction(nullptr)
	, MoveAction(nullptr)
	, LookAction(nullptr)
	, SlideAction(nullptr)
	, InputMoveVector(FVector2D::ZeroVector)
	, JumpDelayMaxTime(0.f)
	, JumpDelayTime(0.f)
	, bIsDelayingJump(false)
	, bIsJumpReleased(true)
	, bIsInputMoveLocked(false)
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

	CameraMovementComponent = CreateDefaultSubobject<UCameraMovementComponent>(TEXT("CameraMovementComponent"));
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

	if (UDeftCharacterMovementComponent* deftCharacterMovementComponent = Cast<UDeftCharacterMovementComponent>(GetCharacterMovement()))
		deftCharacterMovementComponent->OnLandedFromAir.AddUObject(this, &ADeftPlayerCharacter::OnLandedBeginJumpDelay);

	bIsJumpReleased = true;
	JumpDelayMaxTime = 0.2f;
	JumpDelayTime = JumpDelayMaxTime;
}

bool ADeftPlayerCharacter::CanJumpInternal_Implementation() const
{
	return JumpIsAllowedInternal();
}

void ADeftPlayerCharacter::Move(const FInputActionValue& aValue)
{
	if (bIsInputMoveLocked)
	{
		//UE_LOG(LogTemp, Error, TEXT("InputLocked but attempting to move! Cannot move with WASD"));
		return;
	}

	InputMoveVector = aValue.Get<FVector2D>();
	// TODO: add custom player controller
	if (Controller)
	{
		// find out which way is forward
		const FRotator rotation = Controller->GetControlRotation();
		const FRotator yawRotation(0, rotation.Yaw, 0);

		// get forward vector
		// note: is there a benefit from getting forward form the rotation and not just ActorForwardVector?
		const FVector forwardDir = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::X);
		const FVector rightDir = FRotationMatrix(yawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(forwardDir, InputMoveVector.Y);
		AddMovementInput(rightDir, InputMoveVector.X);
	}
}

void ADeftPlayerCharacter::Look(const FInputActionValue& aValue)
{
	FVector2D input = aValue.Get<FVector2D>();

	if (Controller)
	{
		AddControllerYawInput(input.X);
		AddControllerPitchInput(input.Y);
	}
}

void ADeftPlayerCharacter::Slide()
{
	bIsInputMoveLocked = true;

	if (UDeftCharacterMovementComponent* deftCharacterMovementComponent = Cast<UDeftCharacterMovementComponent>(GetCharacterMovement()))
		deftCharacterMovementComponent->DoSlide();
}

void ADeftPlayerCharacter::StopSlide()
{
	bIsInputMoveLocked = false; //TODO: I saw a random bug where movement got locked when is shouldn't after doing a bunch of sequential jumps and slides
}

void ADeftPlayerCharacter::OnLandedBeginJumpDelay()
{
	bIsDelayingJump = true;
	JumpDelayTime = 0.f;
}

void ADeftPlayerCharacter::UpdateJumpDelay(float aDeltaTime)
{
	if (!bIsDelayingJump)
		return;

	if (JumpDelayTime == JumpDelayMaxTime)
		return;

	JumpDelayTime = FMath::Min(JumpDelayTime + aDeltaTime, JumpDelayMaxTime);
}

bool ADeftPlayerCharacter::CanBeginJump()
{
	// note: Removing jump delay for now, if it's desired uncomment this 
	return bIsJumpReleased;//&& JumpDelayTime == JumpDelayMaxTime;
}

// TODO: there is a bug where you can jump while colliding horizontally with a wall and effectively climb up the entire wall
// I think it's probably because we're in the MOVE_Flying movement mode which will allow movement from input
// To fix this I think we may need to manually detect collision in a full 360 around the capsul then change the movement mode to Falling (our falling)
void ADeftPlayerCharacter::BeginJumpProxy()
{
	// Default UE jump logic allows sequential jumps if the user holds down the jump button
	// but I don't like that, so only allow a jump to be processed if they've released the previous
	if (CanBeginJump())
	{
		bIsJumpReleased = false;
		bIsDelayingJump = false;
		Jump();
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Cannot Jump!"));
}

void ADeftPlayerCharacter::StopJumpProxy()
{
	bIsJumpReleased = true;
	StopJumping();
}

// Called every frame
void ADeftPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateJumpDelay(DeltaTime);
}

// Called to bind functionality to input
void ADeftPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* inputComp = static_cast<UEnhancedInputComponent*>(PlayerInputComponent))
	{
		// Jump - TODO: perhaps write my own jump functions
		inputComp->BindAction(JumpAction, ETriggerEvent::Started, this, &ADeftPlayerCharacter::BeginJumpProxy);
		inputComp->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADeftPlayerCharacter::StopJumpProxy);

		// Move
		inputComp->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADeftPlayerCharacter::Move);
		inputComp->BindAction(MoveAction, ETriggerEvent::Completed, this, &ADeftPlayerCharacter::Move);

		// Look
		inputComp->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADeftPlayerCharacter::Look);

		// Slide
		inputComp->BindAction(SlideAction, ETriggerEvent::Started, this, &ADeftPlayerCharacter::Slide);
	}
}

