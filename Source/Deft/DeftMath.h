// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class DEFT_API DeftMath
{
public:
	DeftMath();
	~DeftMath();

	enum class AxisOfRotation { AOR_X, AOR_Y, AOR_Z };

	struct RotMatrix
	{
		RotMatrix(float aDegrees)
			: Degrees(aDegrees)
		{
			Cos = FMath::Cos(FMath::DegreesToRadians(Degrees));
			Sin = FMath::Sin(FMath::DegreesToRadians(Degrees));
		}
		void Rotate(const FVector& v, FVector& outRotated, AxisOfRotation anAxis);
		void AroundX(float(*outMatrix)[3][3]);
		void AroundY(float(*outMatrix)[3][3]);
		void AroundZ(float(*outMatrix)[3][3]);
	private:
		float Degrees;
		float Cos;
		float Sin;
	};
};