// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GenericTeamAgentInterface.h"
#include "TeamIdentityComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AIMASSIST_API UTeamIdentityComponent : public UActorComponent, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UTeamIdentityComponent();

	virtual void SetGenericTeamId(const FGenericTeamId& TeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const { return Team; }

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Team")
	FGenericTeamId Team;
};
