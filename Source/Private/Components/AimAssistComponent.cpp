// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AimAssistComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HUD/AimAssistHUD.h"
#include "Interfaces/AimTargetInterface.h"
#include "Types/AimAssistData.h"

// Sets default values for this component's properties
UAimAssistComponent::UAimAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAimAssistEnabled = false;

	OverlapBoxHalfSize = FVector(50.0f, 500.0f, 500.0f);
	OverlapRange = 1000.0f;
	OffsetFromCenter = FVector2D::ZeroVector;
	ObjectTypesToQuery = { ECC_WorldDynamic, ECC_Pawn };

	bEnableFriction = true;
	FrictionRadius = 100.0f;
	CurrentAimFriction = 0.0f;

	bEnableMagnetism = false;
	MagnetismRadius = 45.0f;
	CurrentAimMagnetism = 0.0f;
}

// Called when the game starts
void UAimAssistComponent::BeginPlay()
{
	Super::BeginPlay();

	EnableAimAssist(false);

	// Cache player controller and camera manager
	PlayerController = Cast<APlayerController>(GetOwner());
	check(PlayerController);

	PlayerCameraManager = PlayerController->PlayerCameraManager;
	check(PlayerCameraManager);

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

	// Ensure that controller is valid all the time and is locally controlled.
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		return;

	// Clear the pervious best target data
	BestTargetData = FAimTargetData{};

	// Get list of valid targets
	const TArray<FAimAssistTarget> ValidTargetList = GetValidTargets();

	if (!ValidTargetList.IsEmpty())
	{
		// find the closest target
		FAimTargetData TempData;
		FindBestFrontFacingTarget(ValidTargetList, BestTargetData);

		if (IsValid(BestTargetData.Component))
		{
			// TODO: Apply aim modifiers
			UE_LOG(LogTemp, Log, TEXT("VALID BEST TARGET -> %s"), *BestTargetData.Component->GetName());
		}
	}

	//FAimTargetData OutCenterMostTarget;
	//const bool bCenterMostTargetFound = FindCenterMostTarget(AimTargetList, OutCenterMostTarget);

	//// Calculate assist modifiers
	//CalculateFriction(OutCenterMostTarget);
	//CalculateMagnetism(OutCenterMostTarget);

	//if (bCenterMostTargetFound)
	//{
	//	UKismetSystemLibrary::DrawDebugString(GetWorld(), OutCenterMostTarget.HitComponent->GetComponentLocation() + (FVector::UpVector * 20.0f), "Active Target", nullptr, FLinearColor::Green);
	//	const FVector CurrentTargetDirection = (OutCenterMostTarget.HitComponent->GetComponentLocation() - PlayerCameraManager->GetCameraLocation()).GetSafeNormal();

	//	// Apply modifiers
	//	ApplyMagnetism(DeltaTime, OutCenterMostTarget.HitComponent->GetComponentLocation(), CurrentTargetDirection);
	//}

	//// Debug draw shapes
	//RequestDebugAimAssistCircles();
}

void UAimAssistComponent::EnableAimAssist(bool bEnabled)
{
	SetComponentTickEnabled(bEnabled);
	bAimAssistEnabled = bEnabled;
}

TArray<FAimAssistTarget> UAimAssistComponent::GetValidTargets()
{
	TArray<FAimAssistTarget> ValidTargets;

	// Get the largest radius from all of aim assist components
	const TArray<float> AimAssistZones{ FrictionRadius, MagnetismRadius };
	const float LargestAimAssistZone = FMath::Max(AimAssistZones);

	// Screen center
	int SizeX, SizeY;
	PlayerController->GetViewportSize(SizeX, SizeY);
	FVector2D ScreenCenter = FVector2D(SizeX * 0.5f, SizeY * 0.5f);
	ScreenCenter += OffsetFromCenter;

	// Sweep-Multi by object type
	TArray<FHitResult> OutHits;
	const FVector StartLoc = PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PlayerCameraManager->GetCameraRotation().Vector();
	const FVector EndLoc = StartLoc + (CameraForward * OverlapRange);
	FCollisionQueryParams QueryParams; // Collision query param
	QueryParams.bTraceComplex = false; // Disable complex trace
	QueryParams.AddIgnoredActor(PlayerController->GetPawn()); // Ignore controlled pawn
	GetWorld()->SweepMultiByObjectType(
		OutHits,
		StartLoc, EndLoc,
		PlayerCameraManager->GetCameraRotation().Quaternion(),
		ObjectQueryParams,
		FCollisionShape::MakeBox(OverlapBoxHalfSize),
		QueryParams);

	for (const auto& Hit : OutHits)
	{
		// Skip if the actor does not implement the UAimTargetInterface
		if (!Hit.GetActor()->GetClass()->ImplementsInterface(UAimTargetInterface::StaticClass()))
			continue;

		// Get all the hit assistance targets on actor
		const TArray<FAimAssistTarget> AimAssistTargets = IAimTargetInterface::Execute_GetAimAssistTargets(Hit.GetActor());

		// Loop over the assist targets
		for (const auto& AimAssistTarget : AimAssistTargets)
		{
			// Build target data
			FAimAssistTarget TargetData;
			TargetData.Component = AimAssistTarget.Component;

			// Loop over the socket locations
			for (const auto& Socket : AimAssistTarget.Sockets)
			{
				const FVector SocketLoc = AimAssistTarget.Component->GetSocketLocation(Socket);

				// Check if the socket location is inside the screen aim assist radius
				if (IsTargetWithinScreenCircle(SocketLoc, ScreenCenter, LargestAimAssistZone))
				{
					// Do a Visibility check for that socket location on component
					FHitResult OutVisibilityHit;
					FCollisionQueryParams VisibilityQueryParams;
					VisibilityQueryParams.AddIgnoredActor(PlayerController->GetPawn());
					VisibilityQueryParams.bTraceComplex = false;
					// Debug
					FColor DebugTraceColor = FColor::Green;
					if (GetWorld()->LineTraceSingleByChannel(OutVisibilityHit, StartLoc, SocketLoc, ECC_Visibility, VisibilityQueryParams))
					{
						// Check if the hit component is same as the current aim assist component
						if (OutVisibilityHit.GetComponent() == AimAssistTarget.Component)
						{
							// Add socket to list
							TargetData.Sockets.Add(Socket);
						}
						else
						{
							DebugTraceColor = FColor::Red;
						}

						DrawDebugLine(GetWorld(), StartLoc, OutVisibilityHit.Location, DebugTraceColor, false, 0.0f);
					}
				}
			}

			// Add the target data into the aim list
			ValidTargets.Add(TargetData);
		}
	}

	return ValidTargets;
}

bool UAimAssistComponent::IsTargetWithinScreenCircle(const FVector& TargetLoc, const FVector2D& ScreenPoint, const float Radius)
{
	FVector2D TargetScreenLoc;
	if (!PlayerController->ProjectWorldLocationToScreen(TargetLoc, TargetScreenLoc, true))
		return false;

	// Distance squared between target and point on screen
	const float DistanceSq = FVector2D::DistSquared(TargetScreenLoc, ScreenPoint);

	// Check if the target point falls in the radius
	return DistanceSq <= FMath::Square(Radius);
}

void UAimAssistComponent::FindBestFrontFacingTarget(const TArray<FAimAssistTarget>& Targets, FAimTargetData& OutTargetData)
{
	float BestScore = -FLT_MAX;
	FAimTargetData BestTarget;

	for (const auto& Target : Targets)
	{
		for (const auto& Socket : Target.Sockets)
		{
			const FVector SocketLoc = Target.Component->GetSocketLocation(Socket);
			const FVector ToTarget = SocketLoc - PlayerCameraManager->GetCameraLocation();
			const float Distance = ToTarget.SizeSquared();
			const FVector ToTargetDir = ToTarget.GetSafeNormal();

			// How "in front" the target is
			const float Dot = FVector::DotProduct(PlayerCameraManager->GetCameraRotation().Vector(), ToTargetDir);

			// Combine distance and front-ness into a score, prevent division by 0
			// Currently only takes into account dot product
			const float Score = Dot;

			if (Score > BestScore)
			{
				BestScore = Score;
				BestTarget.Component = Target.Component;
				BestTarget.SocketName = Socket;
				BestTarget.SocketLocation = SocketLoc;
			}
		}
	}
	// Return the best component and location
	OutTargetData = BestTarget;
}

void UAimAssistComponent::CalculateFriction(FAimTargetData Target)
{
	//if (!IsValid(Target.Actor) || bEnableFriction == false)
	//	return;

	//const float FrictionRadiusSq = FMath::Square(FrictionRadius);

	//if (Target.DistanceSqFromScreenCenter < FrictionRadiusSq)
	//{
	//	// Calculate scale depending on how close we are to the center
	//	float FrictionScale = 1.0f - (Target.DistanceSqFromScreenCenter / FrictionRadiusSq);
	//	// Clamp
	//	FrictionScale = FMath::Clamp(FrictionScale, 0.0f, 1.0f);

	//	// Get the value from curve
	//	float FrictionCurveValue;
	//	if (FrictionCurve)
	//		FrictionCurveValue = FrictionCurve->GetFloatValue(FrictionScale);
	//	else
	//		FrictionCurveValue = 0.75f; // Default value in case curve not found


	//	CurrentAimFriction = FrictionCurveValue;
	//	return;
	//}

	//// Else zero out the aim friction
	//CurrentAimFriction = 0.0f;
}

float UAimAssistComponent::GetCurrentAimFriction()
{
	return 1.0f - CurrentAimFriction;
}

void UAimAssistComponent::CalculateMagnetism(FAimTargetData Target)
{
	//if (!IsValid(Target.Actor) || bEnableMagnetism == false)
	//	return;

	//const float MagnetismRadiusSq = FMath::Square(MagnetismRadius);

	//if (Target.DistanceSqFromScreenCenter < MagnetismRadiusSq)
	//{
	//	// Magnetic factor depending on how close to the center of screen
	//	float MagnetismScale = 1.0f - (Target.DistanceSqFromScreenCenter / MagnetismRadiusSq);
	//	// Clamp
	//	MagnetismScale = FMath::Clamp(MagnetismScale, 0.0f, 1.0f);

	//	GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Magnetism Rate For Active Target : %.2f"), MagnetismScale));

	//	// Get the value from curve
	//	float MagnetismCurveValue;
	//	if (MagnetismCurve)
	//		MagnetismCurveValue = MagnetismCurve->GetFloatValue(MagnetismScale);
	//	else
	//		MagnetismCurveValue = 0.5f; // default value

	//	CurrentAimMagnetism = MagnetismCurveValue;
	//	return;
	//}

	//CurrentAimMagnetism = 0.0f;
}

void UAimAssistComponent::ApplyMagnetism(float DeltaTime, const FVector& TargetLocation, const FVector& TargetDirection)
{
	if (CurrentAimMagnetism == 0.0f || bEnableMagnetism == false)
		return;

	// Get current control rotation
	FRotator CurrentRotation = PlayerController->GetControlRotation();
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
	PlayerController->SetControlRotation(NewRotation);
}
