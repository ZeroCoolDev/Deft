#include "ClimbComponent.h"

#include "DeftPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"

TAutoConsoleVariable<bool> CVar_DebugLedgeUp(TEXT("deft.debug.climb.ledgeup"), true, TEXT("draw debugging for ledgeup"), ECVF_Cheat);

UClimbComponent::UClimbComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}

void UClimbComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

#if !UE_BUILD_SHIPPING
	DrawDebug();
#endif // !UE_BUILD_SHIPPING
}


void UClimbComponent::BeginPlay()
{
	Super::BeginPlay();

	DeftCharacter = Cast<ADeftPlayerCharacter>(GetOwner());
	if (DeftCharacter.IsValid())
	{
		DeftCharacter->OnJumpInputPressed.AddUObject(this, &UClimbComponent::LedgeUp);

		if (USpringArmComponent* springArmComponent = DeftCharacter->FindComponentByClass<USpringArmComponent>())
			LedgeHeightMin = springArmComponent->GetComponentLocation().Z - DeftCharacter->GetActorLocation().Z;
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed finding the spring arm component, defaulting to UE eye height"));
			LedgeHeightMin = DeftCharacter->BaseEyeHeight;
		}
	}
	else
		UE_LOG(LogTemp, Error, TEXT("Failed to find DeftPlayerCharacter!"));
}

void UClimbComponent::LedgeUp()
{
	// check if ledge up is possible
		// eye height raycast, if we don't hit anything (distance = edge of capsul + capsulWidth)
		// we're in a jump or fall state
		// there is room on the ledge for us 
			// after eye height check, go that distance out, then down to find floor.
			// then shape sweep at floor location for our capsul size to see if we hit anything

	// enter ledge up state
}
#if !UE_BUILD_SHIPPING
void UClimbComponent::DrawDebug()
{
	if (!CVar_DebugLedgeUp.GetValueOnGameThread())
		return;

	GEngine->AddOnScreenDebugMessage(-1, 0.005, FColor::Yellow, FString::Printf(TEXT("Ledge Height %.2f, actor z %.2f"), LedgeHeightMin, DeftCharacter->GetActorLocation().Z));

	const FVector lineStart = DeftCharacter->GetActorLocation() + FVector(0.f, 0.f, LedgeHeightMin);
	const FVector lineEnd = lineStart + (DeftCharacter->GetActorForwardVector() * 100.f);
	DrawDebugLine(GetWorld(), lineStart, lineEnd, FColor::Yellow);
}
#endif // !UE_BUILD_SHIPPING

