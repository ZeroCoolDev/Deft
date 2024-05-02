#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GrappleComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	enum class GrappleStateEnum
	{
		None,
		Extending,
		Retracting
	};

public:	
	// Sets default values for this component's properties
	UGrappleComponent();

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void DoGrapple();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision)
	class USceneComponent* GrappleAnchor;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void UpdateGrappleAnchorLocation();
	void ProcessGrapple(float aDeltaTime);
	void EndGrapple();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision)
	class USphereComponent* Grapple;

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;

	FVector GrappleMaxReachPoint;
	float GrappleDistanceMax;	// maximum distance grapple can reach
	float GrappleSpeed;			// speed at which the grapple moves

	GrappleStateEnum GrappleState;

	bool bIsGrappleActive;

#if !UE_BUILD_SHIPPING
	void DrawDebug();

	FVector Debug_GrappleLocThisFrame;
	float Debug_GrappleDistance;
#endif //!UE_BUILD_SHIPPING
};
