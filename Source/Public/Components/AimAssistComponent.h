// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AimAssistComponent.generated.h"

class APlayerController;
class APlayerCameraManager;
class AAimAssistHUD;
class USceneComponent;

USTRUCT(BlueprintType)
struct AIMASSIST_API FAimTargetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<USceneComponent> HitComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D ScreenPosition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistanceSqFromScreenCenter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Distance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DotProduct = -1.0f;
};


UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AIMASSIST_API UAimAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAimAssistComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	void EnableAimAssist(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	void GetActorsInAimZone();

	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	bool FindCenterMostTarget(TArray<FAimTargetData> Targets, FAimTargetData& OutTargetData);

	UFUNCTION(BlueprintCallable, Category = "AimAssist|Friction")
	void CalculateFriction(FAimTargetData Target);

	UFUNCTION(BlueprintPure, Category = "AimAssist|Friction")
	float GetCurrentAimFriction();

	UFUNCTION(BlueprintCallable, Category = "AimAssist|Magnetism")
	void CalculateMagnetism(FAimTargetData Target);

	UFUNCTION(BlueprintCallable, Category = "AimAssist|Magnetism")
	void ApplyMagnetism(float DeltaTime, const FVector& TargetLocation, const FVector& TargetDirection);

	UFUNCTION()
	void RequestDebugAimAssistCircles();

protected:
	UPROPERTY(BlueprintReadWrite, Category = "AimAssist")
	TObjectPtr<APlayerController> OwningPlayerController;

	UPROPERTY(BlueprintReadWrite, Category = "AimAssist")
	TObjectPtr<APlayerCameraManager> OwningPlayerCameraManager;

	UPROPERTY()
	TObjectPtr<AAimAssistHUD> DebugHUD;

	UPROPERTY(BlueprintReadOnly, Category = "AimAssist")
	bool bAimAssistEnabled;

	UPROPERTY(BlueprintReadWrite, Category = "AimAssist")
	TArray<FAimTargetData> AimTargetList;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist")
	FVector OverlapBoxHalfSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist")
	float OverlapRange;

	/** Collision query */
	UPROPERTY(EditDefaultsOnly, Category = "AimAssist")
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypesToQuery;

	// Container for collision object types
	FCollisionObjectQueryParams ObjectQueryParams;


	/** Friction section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction")
	bool bEnableFriction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction")
	float FrictionRadius;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction")
	TObjectPtr<UCurveFloat> FrictionCurve;

	UPROPERTY()
	float CurrentAimFriction;

	/** Magnetism section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Magnetism")
	bool bEnableMagnetism;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Magnetism")
	float MagnetismRadius;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Magnetism")
	TObjectPtr<UCurveFloat> MagnetismCurve;

	UPROPERTY(BlueprintReadOnly, Category = "AimAssist|Magnetism")
	float CurrentAimMagnetism;

	/** Debug section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Debug")
	bool bDrawDebug;
};
