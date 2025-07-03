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

	UPROPERTY(EditDefaultsOnly, Category = "AimAssist")
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypesToQuery;

	FCollisionObjectQueryParams ObjectQueryParams;

	/** Friction section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction")
	float FrictionRadius;

	UPROPERTY(BlueprintReadOnly, Category = "AimAssist|Friction")
	float CurrentAimFriction;

	/** Stickness section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Stickness")
	float SticknessRadius;

	/** Debug section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Debug")
	bool bDrawDebug;
};
