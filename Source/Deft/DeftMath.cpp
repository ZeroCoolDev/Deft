// Fill out your copyright notice in the Description page of Project Settings.


#include "DeftMath.h"

DeftMath::DeftMath()
{
}

DeftMath::~DeftMath()
{
}

void DeftMath::RotMatrix::Rotate(const FVector& v, FVector& outRotated, DeftMath::AxisOfRotation anAxis)
{
	float rotMatrix[3][3];
	switch (anAxis)
	{
	case AxisOfRotation::AOR_X: AroundX(&rotMatrix);
		break;
	case AxisOfRotation::AOR_Y: AroundY(&rotMatrix);
		break;
	case AxisOfRotation::AOR_Z: AroundZ(&rotMatrix);
		break;
	}
	float x = (v.X * rotMatrix[0][0]) + (v.Y * rotMatrix[1][0]) + (v.Z * rotMatrix[2][0]);
	float y = (v.X * rotMatrix[0][1]) + (v.Y * rotMatrix[1][1]) + (v.Z * rotMatrix[2][1]);
	float z = (v.X * rotMatrix[0][2]) + (v.Y * rotMatrix[1][2]) + (v.Z * rotMatrix[2][2]);
	outRotated = FVector(x, y, z);
}

void DeftMath::RotMatrix::AroundX(float(*outMatrix)[3][3])
{
	// X axis 
	(*outMatrix)[0][0] = 1;
	(*outMatrix)[0][1] = 0;
	(*outMatrix)[0][2] = 0;

	// Y axis 
	(*outMatrix)[1][0] = 0;
	(*outMatrix)[1][1] = Cos;
	(*outMatrix)[1][2] = Sin;

	// z axis 
	(*outMatrix)[2][0] = 0;
	(*outMatrix)[2][1] = -Sin;
	(*outMatrix)[2][2] = Cos;
}

void DeftMath::RotMatrix::AroundY(float(*outMatrix)[3][3])
{
	// X axis 
	(*outMatrix)[0][0] = Cos;
	(*outMatrix)[0][1] = 0;
	(*outMatrix)[0][2] = -Sin;

	// Y axis 
	(*outMatrix)[1][0] = 0;
	(*outMatrix)[1][1] = 1;
	(*outMatrix)[1][2] = 0;

	// z axis 
	(*outMatrix)[2][0] = Sin;
	(*outMatrix)[2][1] = 0;
	(*outMatrix)[2][2] = Cos;
}

void DeftMath::RotMatrix::AroundZ(float(*outMatrix)[3][3])
{
	//	// X axis 
	(*outMatrix)[0][0] = Cos;
	(*outMatrix)[0][1] = Sin;
	(*outMatrix)[0][2] = 0;

	// Y axis 
	(*outMatrix)[1][0] = -Sin;
	(*outMatrix)[1][1] = Cos;
	(*outMatrix)[1][2] = 0;

	// z axis 
	(*outMatrix)[2][0] = 0;
	(*outMatrix)[2][1] = 0;
	(*outMatrix)[2][2] = 1;
}

/*
// taken from FVector::RotateAngleAxis just to compare and confirm we're rotating the correct pos/neg directions
// always rotate positively CCW
{
	float S, C;
	FMath::SinCos(&S, &C, aAngle);

	const float XX = Axis.X * Axis.X;
	const float YY = Axis.Y * Axis.Y;
	const float ZZ = Axis.Z * Axis.Z;

	const float XY = Axis.X * Axis.Y;
	const float YZ = Axis.Y * Axis.Z;
	const float ZX = Axis.Z * Axis.X;

	const float XS = Axis.X * S;
	const float YS = Axis.Y * S;
	const float ZS = Axis.Z * S;

	const float OMC = 1.f - C;

	FVector rotated = FVector(
		(OMC * XX + C) * V.X + (OMC * XY - ZS) * V.Y + (OMC * ZX + YS) * V.Z,
		(OMC * XY + ZS) * V.X + (OMC * YY + C) * V.Y + (OMC * YZ - XS) * V.Z,
		(OMC * ZX - YS) * V.X + (OMC * YZ + XS) * V.Y + (OMC * ZZ + C) * V.Z
		);
}
*/