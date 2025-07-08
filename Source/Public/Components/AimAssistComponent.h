// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/InputDeviceSubsystem.h"
#include "AimAssistComponent.generated.h"

class APlayerController;
class APlayerCameraManager;
class USceneComponent;
struct FAimAssistTarget;

USTRUCT(BlueprintType)
struct FAimTargetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "AimTargetData")
	UPrimitiveComponent* Component = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "AimTargetData")
	FName SocketName;

	UPROPERTY(BlueprintReadWrite, Category = "AimTargetData")
	FVector SocketLocation;
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

	/** Enable aim assist */
	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	void EnableAimAssist(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "AimAssist")
	bool IsUsingGamepad();

	UFUNCTION()
	void OnHardwareDeviceChanged(const FPlatformUserId UserId, const FInputDeviceId DeviceId);

	/** Gets all the valid target sockets depending on Query object type and team identifier component */
	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	TArray<FAimAssistTarget> GetValidTargets();

	/**
	* @brief Checks if the provided world location is intersecting with the on screen circle
	*/
	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	bool IsTargetWithinScreenCircle(const FVector& TargetLoc, const FVector2D& ScreenPoint, const float Radius);

	// The the closest and near to the center target from players POV
	UFUNCTION(BlueprintCallable, Category = "AimAssist")
	void FindBestFrontFacingTarget(const TArray<FAimAssistTarget>& Targets, FAimTargetData& OutTargetData);

	/**
	* @return returns the center of screen
	*/
	UFUNCTION(BlueprintPure, Category = "AimAssist")
	FVector2D GetViewportCenter();

	// Calculate the friction factor depending on the distance form the center of circle
	UFUNCTION(BlueprintCallable, Category = "AimAssist|Friction")
	void CalculateFriction(FAimTargetData Target, float DistanceSqFromOrigin);

	// Converts the friction factor into easily useable friction scale (1 - FrictionFactor)
	UFUNCTION(BlueprintPure, Category = "AimAssist|Friction")
	float GetCurrentAimFriction();

	UFUNCTION(BlueprintCallable, Category = "AimAssist|Magnetism")
	void CalculateMagnetism(FAimTargetData Target, float DistanceSqFromOrigin);

	UFUNCTION(BlueprintCallable, Category = "AimAssist|Magnetism")
	void ApplyMagnetism(float DeltaTime, const FVector& TargetLocation, const FVector& TargetDirection);

protected:
	UPROPERTY(BlueprintReadWrite, Category = "AimAssist")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(BlueprintReadWrite, Category = "AimAssist")
	TObjectPtr<APlayerCameraManager> PlayerCameraManager;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "AimAssist")
	bool bAimAssistEnabled;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist")
	bool bUseOnlyOnGamepad;

	UPROPERTY()
	EHardwareDevicePrimaryType LastInputDevice;
	
	UPROPERTY(BlueprintReadOnly, Category = "AimAssist")
	FAimTargetData BestTargetData;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist", meta = (EditCondition = "bAimAssistEnabled"))
	FVector OverlapBoxHalfSize;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist", meta = (EditCondition = "bAimAssistEnabled", ClampMin = "25.0"))
	float OverlapRange;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist", meta = (EditCondition = "bAimAssistEnabled"))
	FVector2D OffsetFromCenter;

	/** Collision query */
	UPROPERTY(EditDefaultsOnly, Category = "AimAssist|Filter", meta = (EditCondition = "bAimAssistEnabled"))
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypesToQuery;

	// Container for collision object types
	FCollisionObjectQueryParams ObjectQueryParams;

	/** Team Id query */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AimAssist|Filter", meta = (EditCondition = "bAimAssistEnabled"))
	bool bQueryForTeams = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AimAssist|Filter", meta = (EditCondition = "bAimAssistEnabled && bQueryForTeams"))
	bool bGetTeamFromNativeInterface = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "AimAssist|Filter", meta = (EditCondition = "bAimAssistEnabled && bQueryForTeams"))
	TArray<FGenericTeamId> TeamsToQuery;

	/** Friction section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction", meta = (EditCondition = "bAimAssistEnabled"))
	bool bEnableFriction;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction", meta = (EditCondition = "bAimAssistEnabled && bEnableFriction", ClampMin = "10.0"))
	float FrictionRadius;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Friction", meta = (EditCondition = "bAimAssistEnabled && bEnableFriction"))
	TObjectPtr<UCurveFloat> FrictionCurve;

	// Aim friction
	UPROPERTY(BlueprintReadOnly, Category = "AimAssist|Friction")
	float CurrentAimFriction;

	/** Magnetism section */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Magnetism", meta = (EditCondition = "bAimAssistEnabled"))
	bool bEnableMagnetism;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Magnetism", meta = (EditCondition = "bAimAssistEnabled && bEnableMagnetism", ClampMin = "10.0"))
	float MagnetismRadius;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Magnetism", meta = (EditCondition = "bAimAssistEnabled && bEnableMagnetism"))
	TObjectPtr<UCurveFloat> MagnetismCurve;

	UPROPERTY(BlueprintReadOnly, Category = "AimAssist|Magnetism")
	float CurrentAimMagnetism;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "AimAssist|Debug")
	bool bShowDebug;
};
