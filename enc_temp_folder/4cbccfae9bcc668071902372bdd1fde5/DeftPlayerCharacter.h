#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "DeftPlayerCharacter.generated.h"

UCLASS()
class DEFT_API ADeftPlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ADeftPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	const FVector2D& GetInputMoveVector() const { return InputMoveVector; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Move(const FInputActionValue& aValue);
	void Look(const FInputActionValue& aValue);
	void Slide();

	void OnLandedBeginJumpDelay();
	void UpdateJumpDelay(float aDeltaTim);
	bool CanBeginJump();
	void BeginJumpProxy();
	void StopJumpProxy();

	// Spring arm component to follow the camera camera behind the player
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	class USpringArmComponent* SpringArmComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Camera)
	class UCameraComponent* CameraComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	class UInputAction* SlideAction;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	class UCameraMovementComponent* CameraMovementComponent;

private:
	FVector2D InputMoveVector;
	
	float JumpDelayMaxTime;
	float JumpDelayTime;
	bool bIsDelayingJump;

	bool bIsJumpReleased;
};