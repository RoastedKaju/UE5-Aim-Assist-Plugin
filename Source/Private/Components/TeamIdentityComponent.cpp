// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TeamIdentityComponent.h"

// Sets default values for this component's properties
UTeamIdentityComponent::UTeamIdentityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	Team = FGenericTeamId::NoTeam;
}


// Called when the game starts
void UTeamIdentityComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UTeamIdentityComponent::SetGenericTeamId(const FGenericTeamId& TeamID)
{
	Team = TeamID;
}
