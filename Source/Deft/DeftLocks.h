// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class DEFT_API DeftLocks
{
public:
	DeftLocks(){}
	~DeftLocks(){}

	static bool IsSlideLocked();
	static void IncrementSlideLockRef();
	static void DecrementSlideLockRef();

	static bool IsInputLocked();
	static void IncrementInputLockRef();
	static void DecrementInputLockRef();

private:
	static int SlideLock;
	static int InputLock;
};
