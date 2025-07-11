// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AimAssistData.generated.h"

USTRUCT(BlueprintType)
struct AIMASSIST_API FAimAssistTarget
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UPrimitiveComponent> Component;

    UPROPERTY(BlueprintReadWrite)
    TArray<FName> Sockets;
};