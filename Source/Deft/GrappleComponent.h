#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "GrappleComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnGrapplePull, bool/*bStarted*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFT_API UGrappleComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
	enum class GrappleStateEnum
	{
		None,
		Extending,
		Retracting,
		Pulling
	};

public:	
	UGrappleComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void DoGrapple();

	FOnGrapplePull OnGrapplePullDelegate;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void UpdateGrappleAnchorLocation();
	void ProcessGrapple(float aDeltaTime);


	void ExtendGrapple(float aDeltaTime);
	void PullGrapple(float aDeltaTime);
	void EndGrapple(bool aApplyImpulse, AActor* aHitActor = nullptr);

	float CalculateAngleToReach(const FVector& aTargetLocation);
	bool CalculatePath(float aImpulseAngle);

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision)
	class USceneComponent* GrappleAnchor;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Collision)
	class USphereComponent* Grapple;

	TWeakObjectPtr<class ADeftPlayerCharacter> DeftCharacter;

	// Extending
	FVector GrappleMaxReachPoint;
	float GrappleReachThreshold;	// how close the grapple needs to actually get to the max reach before we consider it complete
	float GrappleDistanceMax;		// maximum distance grapple can reach
	float GrappleExtendSpeed;		// speed at which the grapple moves
	bool bIsGrappleExtendActive;

	// Pulling
	TArray<FVector> GrapplePullPath;				// the entire path we should travel for the grapple
	int GrapplePullIndex;							// the current point in the path we're travelling to
	TWeakObjectPtr<class AActor> AttachedActor;		// who/what is being pulled (player, enemy, box...etc)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Collision)
	float GrapplePullSpeed;							// speed at which the player is pulled by the grapple
	float PullTime;
	float PullTimeMaxTime;
	bool bIsGrapplePullActive;

	GrappleStateEnum GrappleState;

#if !UE_BUILD_SHIPPING
	void DrawDebug();

	FVector Debug_GrappleLocThisFrame;
	FVector Debug_GrappleMaxLocReached;
	float Debug_GrappleDistance;
	float Debug_GrappleLaunchDeg1;
	float Debug_GrappleLaunchDeg2;
#endif //!UE_BUILD_SHIPPING
};
