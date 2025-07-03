// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AimAssistHUD.generated.h"

USTRUCT()
struct AIMASSIST_API FLineRequestData
{
	GENERATED_BODY()

	float StartX;
	float StartY;
	float EndX;
	float EndY;
	FLinearColor Color;
	float Thickness;

	FLineRequestData() = default;
	FLineRequestData(float StartX, float StartY, float EndX, float EndY, FLinearColor LineColor, float LineThickness = 0.0f) :
		StartX(StartX),
		StartY(StartY),
		EndX(EndX),
		EndY(EndY),
		Color(LineColor),
		Thickness(LineThickness)
	{

	}
};

/**
 *
 */
UCLASS()
class AIMASSIST_API AAimAssistHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	void RequestCircleOutline(FVector2D ScreenPos, FLinearColor Color = FLinearColor::Red, float Radius = 50.0f, int NumSegments = 32, float Thickness = 1.0f);

protected:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void DrawCircleOutline(FVector2D ScreenPos, FLinearColor Color = FLinearColor::Red, float Radius = 50.0f, int NumSegments = 32, float Thickness = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void DrawRectOutline(float PosX, float PosY, float Width, float Height, FLinearColor Color = FLinearColor::Red, float Thickness = 1.0f);

	UFUNCTION(BlueprintPure, Category = "HUD")
	FVector2D GetScreenCenter() const;

	FVector2D ScreenSize;

private:
	UPROPERTY()
	TArray<FLineRequestData> DebugLineRequests;
};
