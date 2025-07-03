// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/AimAssistHUD.h"
#include "Engine/Canvas.h"

void AAimAssistHUD::DrawHUD()
{
	Super::DrawHUD();

	ScreenSize = FVector2D(Canvas->SizeX, Canvas->SizeY);

	for (const auto& Request : DebugLineRequests)
	{
		DrawLine(Request.StartX, Request.StartY, Request.EndX, Request.EndY, Request.Color, Request.Thickness);
	}

	// Clear out the requests
	DebugLineRequests.Empty();
}

void AAimAssistHUD::RequestCircleOutline(FVector2D ScreenPos, FLinearColor Color, float Radius, int NumSegments, float Thickness)
{
	for (int32 i = 0; i < NumSegments; i++)
	{
		float Angle1 = (2 * PI) * (i / (float)NumSegments);
		float Angle2 = (2 * PI) * ((i + 1) / (float)NumSegments);

		FVector2D Point1 = ScreenPos + FVector2D(FMath::Cos(Angle1), FMath::Sin(Angle1)) * Radius;
		FVector2D Point2 = ScreenPos + FVector2D(FMath::Cos(Angle2), FMath::Sin(Angle2)) * Radius;

		DebugLineRequests.Add(FLineRequestData(Point1.X, Point1.Y, Point2.X, Point2.Y, Color, Thickness));
	}
}

void AAimAssistHUD::DrawCircleOutline(FVector2D ScreenPos, FLinearColor Color, float Radius, int NumSegments, float Thickness)
{
	for (int32 i = 0; i < NumSegments; i++)
	{
		float Angle1 = (2 * PI) * (i / (float)NumSegments);
		float Angle2 = (2 * PI) * ((i + 1) / (float)NumSegments);

		FVector2D Point1 = ScreenPos + FVector2D(FMath::Cos(Angle1), FMath::Sin(Angle1)) * Radius;
		FVector2D Point2 = ScreenPos + FVector2D(FMath::Cos(Angle2), FMath::Sin(Angle2)) * Radius;

		DrawLine(Point1.X, Point1.Y, Point2.X, Point2.Y, Color, Thickness);
	}
}

void AAimAssistHUD::DrawRectOutline(float PosX, float PosY, float Width, float Height, FLinearColor Color, float Thickness)
{
	DrawLine(PosX, PosY, PosX + Width, PosY, Color, Thickness);
	DrawLine(PosX, PosY + Height, PosX + Width, PosY + Height, Color, Thickness);
	DrawLine(PosX, PosY, PosX, PosY + Height, Color, Thickness);
	DrawLine(PosX + Width, PosY, PosX + Width, PosY + Height, Color, Thickness);
}

FVector2D AAimAssistHUD::GetScreenCenter() const
{
	return ScreenSize * 0.5f;
}
