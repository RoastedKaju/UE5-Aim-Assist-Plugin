// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/TeamIdentityComponent.h"

// Sets default values for this component's properties
UTeamIdentityComponent::UTeamIdentityComponent() : Team(FGenericTeamId::NoTeam)
{
	PrimaryComponentTick.bCanEverTick = false;
}
