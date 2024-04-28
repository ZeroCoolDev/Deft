// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class DEFT_API DeftLocks
{
private:
	static uint8 CameraMovementDipLock;
	static uint8 SlideLock;
	static uint8 InputLock;

public:
	DeftLocks(){}
	~DeftLocks(){}

	static bool IsCameraMovementDipLocked();
	static void IncrementCameraMovementDipLockRef();
	static void DecrementCamreaMovementDipLockRef();

	static bool IsSlideLocked();
	static void IncrementSlideLockRef();
	static void DecrementSlideLockRef();

	static bool IsInputLocked();
	static void IncrementInputLockRef();
	static void DecrementInputLockRef();

#if !UE_BUILD_SHIPPING
	static void DrawLockDebug();
#endif //!UE_BUILD_SHIPPING
};
