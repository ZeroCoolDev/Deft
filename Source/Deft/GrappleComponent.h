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
	UGrappleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void DoGrapple();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void UpdateGrappleAnchorLocation();
	void ProcessGrapple(float aDeltaTime);

	void ExtendGrapple(float aDeltaTime);
	void EndGrapple(bool aApplyImpulse);

	void CalculateAngleToReach(const FVector& aTargetLocation);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision)
	class USceneComponent* GrappleAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision)
	class USphereComponent* Grapple;

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;

	FVector GrappleMaxReachPoint;
	float GrappleReachThreshold;	// how close the grapple needs to actually get to the max reach before we consider it complete
	float GrappleDistanceMax;		// maximum distance grapple can reach
	float GrappleExtendSpeed;		// speed at which the grapple moves

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision)
	float GrapplePullSpeed;			// speed at which the player is pulled by the grapple

	GrappleStateEnum GrappleState;

	bool bIsGrappleActive;

	TArray<FVector> PredictedLine;

#if !UE_BUILD_SHIPPING
	void DrawDebug();

	FVector Debug_GrappleLocThisFrame;
	FVector Debug_GrappleMaxLocReached;
	float Debug_GrappleDistance;
	float Debug_GrappleLaunchDeg1;
	float Debug_GrappleLaunchDeg2;
#endif //!UE_BUILD_SHIPPING
};
