#include "DeftLocks.h"

int DeftLocks::SlideLock = 0;
int DeftLocks::InputLock = 0;

bool DeftLocks::IsSlideLocked(){ return SlideLock > 0; }
void DeftLocks::IncrementSlideLockRef(){ ++SlideLock; }
void DeftLocks::DecrementSlideLockRef(){ SlideLock = FMath::Max(0, --SlideLock); }

bool DeftLocks::IsInputLocked() { return InputLock > 0; }
void DeftLocks::IncrementInputLockRef(){ ++InputLock; }
void DeftLocks::DecrementInputLockRef() { InputLock = FMath::Max(0, --InputLock); }
