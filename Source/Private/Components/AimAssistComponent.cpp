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

	EnableAimAssist(false);

	// Cache player controller and camera manager
	OwningPlayerController = Cast<APlayerController>(GetOwner());
	check(OwningPlayerController);

	OwningPlayerCameraManager = OwningPlayerController->PlayerCameraManager;
	check(OwningPlayerCameraManager);

	// Setup the object query params
	for (const auto& ObjectType : ObjectTypesToQuery)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ObjectType);
	}

	// Get HUD for drawing debug elements on screen
	DebugHUD = Cast<AAimAssistHUD>(OwningPlayerController->GetHUD());
}

// Called every frame
void UAimAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ensure that controller is valid all the time and is locally controlled.
	if (!IsValid(OwningPlayerController) || !OwningPlayerController->IsLocalController())
		return;

	// Get list of valid targets
	const TArray<FAimAssistTarget> ValidTargetList = GetValidTargets();

	if (!ValidTargetList.IsEmpty())
	{
		// find the closest target
	}

	// Hard exit
	return;

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

	// Debug draw shapes
	RequestDebugAimAssistCircles();
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
	// TODO: Make it a visible property users can change
	int SizeX, SizeY;
	OwningPlayerController->GetViewportSize(SizeX, SizeY);
	FVector2D ScreenCenter = FVector2D(SizeX * 0.5f, SizeY * 0.5f);

	// Sweep-Multi by object type
	TArray<FHitResult> OutHits;
	const FVector StartLoc = OwningPlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = OwningPlayerCameraManager->GetCameraRotation().Vector();
	const FVector EndLoc = StartLoc + (CameraForward * OverlapRange);
	FCollisionQueryParams QueryParams; // Collision query param
	QueryParams.bTraceComplex = false; // Disable complex trace
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
				if (IsLocationInsideScreenCircle(SocketLoc, ScreenCenter, LargestAimAssistZone))
				{
					// Do a Visibility check for that socket location on component
					FHitResult OutVisibilityHit;
					FCollisionQueryParams VisibilityQueryParams;
					VisibilityQueryParams.AddIgnoredActor(OwningPlayerController->GetPawn());
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
					DrawDebugString(GetWorld(), SocketLoc, Socket.ToString(), nullptr, FColor::White, 0.0f);
				}
			}

			// Add the target data into the aim list
			ValidTargets.Add(TargetData);
		}
	}

	return ValidTargets;
}

bool UAimAssistComponent::IsLocationInsideScreenCircle(const FVector& TargetLoc, const FVector2D& ScreenPoint, const float Radius)
{
	FVector2D TargetScreenLoc;
	if (!OwningPlayerController->ProjectWorldLocationToScreen(TargetLoc, TargetScreenLoc, true))
		return false;

	// Distance squared between target and point on screen
	float DistanceSq = FVector2D::DistSquared(TargetScreenLoc, ScreenPoint);

	// Check if the target point falls in the radius
	if (DistanceSq <= FMath::Square(Radius))
		return true;

	// Else return false
	return false;
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
		// Calculate scale depending on how close we are to the center
		float FrictionScale = 1.0f - (Target.DistanceSqFromScreenCenter / FrictionRadiusSq);
		// Clamp
		FrictionScale = FMath::Clamp(FrictionScale, 0.0f, 1.0f);

		// Get the value from curve
		float FrictionCurveValue;
		if (FrictionCurve)
			FrictionCurveValue = FrictionCurve->GetFloatValue(FrictionScale);
		else
			FrictionCurveValue = 0.75f; // Default value in case curve not found


		CurrentAimFriction = FrictionCurveValue;
		return;
	}

	// Else zero out the aim friction
	CurrentAimFriction = 0.0f;
}

float UAimAssistComponent::GetCurrentAimFriction()
{
	return 1.0f - CurrentAimFriction;
}

void UAimAssistComponent::CalculateMagnetism(FAimTargetData Target)
{
	if (!IsValid(Target.Actor) || bEnableMagnetism == false)
		return;

	const float MagnetismRadiusSq = FMath::Square(MagnetismRadius);

	if (Target.DistanceSqFromScreenCenter < MagnetismRadiusSq)
	{
		// Magnetic factor depending on how close to the center of screen
		float MagnetismScale = 1.0f - (Target.DistanceSqFromScreenCenter / MagnetismRadiusSq);
		// Clamp
		MagnetismScale = FMath::Clamp(MagnetismScale, 0.0f, 1.0f);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Magnetism Rate For Active Target : %.2f"), MagnetismScale));

		// Get the value from curve
		float MagnetismCurveValue;
		if (MagnetismCurve)
			MagnetismCurveValue = MagnetismCurve->GetFloatValue(MagnetismScale);
		else
			MagnetismCurveValue = 0.5f; // default value

		CurrentAimMagnetism = MagnetismCurveValue;
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
