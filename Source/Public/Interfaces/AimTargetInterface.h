// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AimTargetInterface.generated.h"

class USceneComponent;

/*
This class does not need to be modified.
Empty class for reflection system visibility.
Uses the UINTERFACE macro.
Inherits from UInterface.
*/
UINTERFACE(MinimalAPI, Blueprintable)
class UAimTargetInterface : public UInterface
{
	GENERATED_BODY()
};


/* Actual Interface declaration. */
class IAimTargetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Aim Assist")
	TArray<USceneComponent*> GetAimTargets() const;
};