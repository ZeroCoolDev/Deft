#include "DeftLocks.h"

uint8 DeftLocks::CameraMovementDipLock = 0;
uint8 DeftLocks::SlideLock = 0;
uint8 DeftLocks::InputLock = 0;

bool DeftLocks::IsCameraMovementDipLocked() { return CameraMovementDipLock > 0; }
void DeftLocks::IncrementCameraMovementDipLockRef() { ++CameraMovementDipLock; }
void DeftLocks::DecrementCamreaMovementDipLockRef() { CameraMovementDipLock = FMath::Max(uint8(0), --CameraMovementDipLock); }

bool DeftLocks::IsSlideLocked(){ return SlideLock > 0; }
void DeftLocks::IncrementSlideLockRef(){ ++SlideLock; }
void DeftLocks::DecrementSlideLockRef(){ SlideLock = FMath::Max(uint8(0), --SlideLock); }

bool DeftLocks::IsInputLocked() { return InputLock > 0; }
void DeftLocks::IncrementInputLockRef(){ ++InputLock; }
void DeftLocks::DecrementInputLockRef() { InputLock = FMath::Max(uint8(0), --InputLock); }

#if !UE_BUILD_SHIPPING
void DeftLocks::DrawLockDebug()
{
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, DeftLocks::IsCameraMovementDipLocked() ? FColor::Red : FColor::White, FString::Printf(TEXT("\tCam Dip: %u"), CameraMovementDipLock));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, DeftLocks::IsSlideLocked() ? FColor::Red : FColor::White, FString::Printf(TEXT("\tSlide: %u"), SlideLock));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, DeftLocks::IsInputLocked() ? FColor::Red : FColor::White, FString::Printf(TEXT("\tInput: %u"), InputLock));
	GEngine->AddOnScreenDebugMessage(-1, 0.005f, FColor::White, TEXT("\n-Locks-"));
}
#endif//!UE_BUILD_SHIPPING
