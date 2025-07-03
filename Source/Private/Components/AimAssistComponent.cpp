// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AimAssistComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HUD/AimAssistHUD.h"

// Sets default values for this component's properties
UAimAssistComponent::UAimAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAimAssistEnabled = false;

	OverlapBoxHalfSize = FVector(50.0f, 500.0f, 500.0f);
	OverlapRange = 1000.0f;

	FrictionRadius = 100.0f;
	CurrentAimFriction = 0.0f;

	SticknessRadius = 45.0f;

	bDrawDebug = false;
}

// Called when the game starts
void UAimAssistComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickEnabled(false);
	bAimAssistEnabled = false;

	// Cache player controller and camera manager
	OwningPlayerController = Cast<APlayerController>(GetOwner());
	check(OwningPlayerController);

	OwningPlayerCameraManager = OwningPlayerController->PlayerCameraManager;
	check(OwningPlayerCameraManager);

	// Get HUD for drawing debug elements on screen
	DebugHUD = Cast<AAimAssistHUD>(OwningPlayerController->GetHUD());

}

// Called every frame
void UAimAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Debug draw shapes
	RequestDebugAimAssistCircles();

	GetActorsInAimZone();

	FAimTargetData OutCenterMostTarget;
	const bool bCenterMostTargetFound = FindCenterMostTarget(AimTargetList, OutCenterMostTarget);
	if (bCenterMostTargetFound)
	{
		UKismetSystemLibrary::DrawDebugString(GetWorld(), OutCenterMostTarget.Actor->GetActorLocation() + (FVector::UpVector * 20.0f), "Active Target", nullptr, FLinearColor::Green);
	}

	// Calculate assist modifiers
	CalculateFriction(OutCenterMostTarget);
}

void UAimAssistComponent::EnableAimAssist(bool bEnabled)
{
	SetComponentTickEnabled(bEnabled);
	bAimAssistEnabled = bEnabled;
}

void UAimAssistComponent::GetActorsInAimZone()
{
	// Clear the existing targets list
	AimTargetList.Empty();

	// Determine the largest radius from all of aim assist components
	TArray<float> AimAssistZones{ FrictionRadius, SticknessRadius };
	const float LargestAimAssistZone = FMath::Max(AimAssistZones);

	// Screen center
	int SizeX, SizeY;
	OwningPlayerController->GetViewportSize(SizeX, SizeY);
	FVector2D ScreenCenter = FVector2D(SizeX * 0.5f, SizeY * 0.5f);

	// Do a box overlap
	TArray<FHitResult> OutHits;
	const FVector StartLoc = OwningPlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = OwningPlayerCameraManager->GetCameraRotation().Vector();
	const FVector EndLoc = StartLoc + (CameraForward * OverlapRange);
	FCollisionQueryParams QueryParams;
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(OwningPlayerController->GetPawn()); // Ignore controlled pawn
	GetWorld()->SweepMultiByChannel(
		OutHits,
		StartLoc, EndLoc,
		OwningPlayerCameraManager->GetCameraRotation().Quaternion(),
		ECC_GameTraceChannel1,
		FCollisionShape::MakeBox(OverlapBoxHalfSize),
		QueryParams);

	for (const auto& Hit : OutHits)
	{
		FVector2D HitActorScreenLoc;
		if (OwningPlayerController->ProjectWorldLocationToScreen(Hit.GetActor()->GetActorLocation(), HitActorScreenLoc, true))
		{
			auto DistanceFromCenter = FVector2D::DistSquared(HitActorScreenLoc, ScreenCenter);
			// Target Data
			FAimTargetData TargetData;
			TargetData.Actor = Hit.GetActor();
			TargetData.ScreenPosition = HitActorScreenLoc;
			TargetData.DistanceSqFromScreenCenter = DistanceFromCenter;

			// Get all the targets inside the largest aim zone
			if (DistanceFromCenter <= FMath::Square(LargestAimAssistZone))
			{
				UKismetSystemLibrary::DrawDebugLine(GetWorld(), StartLoc, Hit.GetActor()->GetActorLocation(), FLinearColor::Green);
				UKismetSystemLibrary::DrawDebugString(GetWorld(), Hit.GetActor()->GetActorLocation(), "Target");

				// Add the target to list
				AimTargetList.Add(TargetData);
			}
		}

	}

}

bool UAimAssistComponent::FindCenterMostTarget(TArray<FAimTargetData> Targets, FAimTargetData& OutTargetData)
{
	if (Targets.IsEmpty())
		return false;

	FAimTargetData ResultTargetData = *Targets.begin();

	// Loop over the target list and do dot product
	for (auto& Target : Targets)
	{
		const FVector TargetDirection = (Target.Actor->GetActorLocation() - OwningPlayerCameraManager->GetCameraLocation()).GetSafeNormal();
		const FVector CameraDirection = OwningPlayerCameraManager->GetCameraRotation().Vector().GetSafeNormal();
		Target.DotProduct = FVector::DotProduct(CameraDirection, TargetDirection);

		if (Target.DotProduct > ResultTargetData.DotProduct)
		{
			ResultTargetData = Target;
		}
	}

	OutTargetData = ResultTargetData;

	return true;
}

void UAimAssistComponent::CalculateFriction(FAimTargetData Target)
{
	if (!IsValid(Target.Actor))
		return;

	const float FrictionRadiusSq = FMath::Square(FrictionRadius);

	if (Target.DistanceSqFromScreenCenter < FMath::Square(FrictionRadius))
	{
		// Reduce the sensivity when near target, the more closer to center target and more heavy friction
		float FrictionStrength = 1.0f - (Target.DistanceSqFromScreenCenter / FrictionRadiusSq);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Friction Rate For Active Target : %.2f"), FrictionStrength));

		// TODO: Add graph curve for better control

		CurrentAimFriction = FrictionStrength;
		return;
	}

	// Else zero out the aim friction
	CurrentAimFriction = 0.0f;
}

void UAimAssistComponent::RequestDebugAimAssistCircles()
{
	// Draw debug circles on screen
	if (!IsValid(DebugHUD))
	{
		return;
	}

	// Screen center
	int SizeX, SizeY;
	OwningPlayerController->GetViewportSize(SizeX, SizeY);
	FVector2D ScreenCenter = FVector2D(SizeX * 0.5f, SizeY * 0.5f);

	// Friction circle
	DebugHUD->RequestCircleOutline(ScreenCenter, FLinearColor::Red, FrictionRadius);

	// Stickness circle
	DebugHUD->RequestCircleOutline(ScreenCenter, FColor::Purple, SticknessRadius);
}
