// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AimAssistComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HUD/AimAssistHUD.h"
#include "Interfaces/AimTargetInterface.h"

// Sets default values for this component's properties
UAimAssistComponent::UAimAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAimAssistEnabled = false;

	OverlapBoxHalfSize = FVector(50.0f, 500.0f, 500.0f);
	OverlapRange = 1000.0f;
	ObjectTypesToQuery = { ECC_WorldDynamic, ECC_Pawn };

	bEnableFriction = true;
	FrictionRadius = 100.0f;
	CurrentAimFriction = 0.0f;

	bEnableMagnetism = false;
	MagnetismRadius = 45.0f;
	CurrentAimMagnetism = 0.0f;

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

	// Setup the object query params
	for (const auto& ObjectType : ObjectTypesToQuery)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ObjectType);
	}

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

	// Calculate assist modifiers
	CalculateFriction(OutCenterMostTarget);
	CalculateMagnetism(OutCenterMostTarget);

	if (bCenterMostTargetFound)
	{
		UKismetSystemLibrary::DrawDebugString(GetWorld(), OutCenterMostTarget.HitComponent->GetComponentLocation() + (FVector::UpVector * 20.0f), "Active Target", nullptr, FLinearColor::Green);
		const FVector CurrentTargetDirection = (OutCenterMostTarget.HitComponent->GetComponentLocation() - OwningPlayerCameraManager->GetCameraLocation()).GetSafeNormal();
		
		// Apply modifiers
		ApplyMagnetism(DeltaTime, OutCenterMostTarget.HitComponent->GetComponentLocation(), CurrentTargetDirection);
	}
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
	TArray<float> AimAssistZones{ FrictionRadius, MagnetismRadius };
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
	FCollisionQueryParams QueryParams; // Collision query param
	QueryParams.bTraceComplex = false;
	QueryParams.AddIgnoredActor(OwningPlayerController->GetPawn()); // Ignore controlled pawn
	GetWorld()->SweepMultiByObjectType(
		OutHits,
		StartLoc, EndLoc,
		OwningPlayerCameraManager->GetCameraRotation().Quaternion(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(OverlapBoxHalfSize),
		QueryParams);

	for (const auto& Hit : OutHits)
	{
		// Skip if the actor does not implement the UAimTargetInterface
		if (!Hit.GetActor()->GetClass()->ImplementsInterface(UAimTargetInterface::StaticClass()))
			continue;

		// Get all the hit target components on actor
		const TArray<USceneComponent*> AimTargetComps = IAimTargetInterface::Execute_GetAimTargets(Hit.GetActor());
		for (const auto& TargetComp : AimTargetComps)
		{
			const FVector CompWorldLoc = TargetComp->GetComponentLocation();

			// TODO: Visiblity trace from camera to hit component

			FVector2D HitActorScreenLoc;
			if (OwningPlayerController->ProjectWorldLocationToScreen(CompWorldLoc, HitActorScreenLoc, true))
			{
				auto DistanceFromCenter = FVector2D::DistSquared(HitActorScreenLoc, ScreenCenter);
				// Target Data
				FAimTargetData TargetData;
				TargetData.Actor = Hit.GetActor();
				TargetData.HitComponent = TargetComp;
				TargetData.ScreenPosition = HitActorScreenLoc;
				TargetData.DistanceSqFromScreenCenter = DistanceFromCenter;

				// Get all the targets inside the largest aim zone
				if (DistanceFromCenter <= FMath::Square(LargestAimAssistZone))
				{
					UKismetSystemLibrary::DrawDebugLine(GetWorld(), StartLoc, CompWorldLoc, FLinearColor::Green);
					UKismetSystemLibrary::DrawDebugString(GetWorld(), CompWorldLoc, "Target");

					// Add the target to list
					AimTargetList.Add(TargetData);
				}
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
		const FVector TargetDirection = (Target.HitComponent->GetComponentLocation() - OwningPlayerCameraManager->GetCameraLocation()).GetSafeNormal();
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
	if (!IsValid(Target.Actor) || bEnableFriction == false)
		return;

	const float FrictionRadiusSq = FMath::Square(FrictionRadius);

	if (Target.DistanceSqFromScreenCenter < FrictionRadiusSq)
	{
		// Calculate strength depending on how close we are to the center
		float FrictionStrength = 1.0f - (Target.DistanceSqFromScreenCenter / FrictionRadiusSq);
		// Clamp
		FrictionStrength = FMath::Clamp(FrictionStrength, 0.0f, 1.0f);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Friction Rate For Active Target : %.2f"), FrictionStrength));

		// TODO: Add graph curve for better control

		CurrentAimFriction = FrictionStrength;
		return;
	}

	// Else zero out the aim friction
	CurrentAimFriction = 0.0f;
}

void UAimAssistComponent::ApplyFriction()
{

}

void UAimAssistComponent::CalculateMagnetism(FAimTargetData Target)
{
	if (!IsValid(Target.Actor) || bEnableMagnetism == false)
		return;

	const float MagnetismRadiusSq = FMath::Square(MagnetismRadius);

	if (Target.DistanceSqFromScreenCenter < MagnetismRadiusSq)
	{
		// Magnetic factor depending on how close to the center of screen
		float MagnetismStrength = 1.0f - (Target.DistanceSqFromScreenCenter / MagnetismRadiusSq);
		// Clamp
		MagnetismStrength = FMath::Clamp(MagnetismStrength, 0.0f, 1.0f);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Magnetism Rate For Active Target : %.2f"), MagnetismStrength));

		// TODO: Add graph curve for better control

		CurrentAimMagnetism = MagnetismStrength;
		return;
	}

	CurrentAimMagnetism = 0.0f;
}

void UAimAssistComponent::ApplyMagnetism(float DeltaTime, const FVector& TargetLocation, const FVector& TargetDirection)
{
	if (CurrentAimMagnetism == 0.0f || bEnableMagnetism == false)
		return;

	// Get current control rotation
	FRotator CurrentRotation = OwningPlayerController->GetControlRotation();
	FRotator TargetRotation = TargetDirection.Rotation();
	
	// TODO: This is where you should use the values from curve
	float RotationSpeed = FMath::Lerp(1.0f, 5.0f, CurrentAimMagnetism);
	
	FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		RotationSpeed
	);

	// Apply the new rotation
	OwningPlayerController->SetControlRotation(NewRotation);
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

	// Magnetism circle
	DebugHUD->RequestCircleOutline(ScreenCenter, FColor::Purple, MagnetismRadius);
}
