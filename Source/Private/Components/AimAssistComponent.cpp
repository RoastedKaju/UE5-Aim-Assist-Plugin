// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AimAssistComponent.h"
#include "Components/TeamIdentityComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/AimTargetInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "Types/AimAssistData.h"
#include "GameFramework/Pawn.h"

// Sets default values for this component's properties
UAimAssistComponent::UAimAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAimAssistEnabled = false;
	bUseOnlyOnGamepad = true;
	LastInputDevice = EHardwareDevicePrimaryType::Unspecified;

	OverlapBoxHalfSize = FVector(500.0f, 1000.0f, 1000.0f);
	OverlapRange = 2500.0f;
	OffsetFromCenter = FVector2D::ZeroVector;
	ObjectTypesToQuery = {ECC_WorldDynamic, ECC_Pawn};
	bQueryForTeams = true;
	bGetTeamFromNativeInterface = false;
	TeamsToQuery = {FGenericTeamId::NoTeam};

	bEnableFriction = true;
	FrictionRadius = 200.0f;
	CurrentAimFriction = 0.0f;

	bEnableMagnetism = true;
	MagnetismRadius = 75.0f;
	CurrentAimMagnetism = 0.0f;

	bShowDebug = false;
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

	// Set up the object query params
	for (const auto& ObjectType : ObjectTypesToQuery)
	{
		ObjectQueryParams.AddObjectTypesToQuery(ObjectType);
	}

	// Bind the function when hardware input device changes
	const auto& InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>();
	check(InputDeviceSubsystem);
	InputDeviceSubsystem->OnInputHardwareDeviceChanged.AddDynamic(this, &UAimAssistComponent::OnHardwareDeviceChanged);
}

// Called every frame
void UAimAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                        FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ensure that controller is valid all the time and is locally controlled.
	if (!IsValid(PlayerController) || !PlayerController->IsLocalController() || !IsValid(PlayerCameraManager))
		return;

	// Clear the previous best target data
	BestTargetData = FAimTargetData{};
	// Reset the friction and magnetism values
	CurrentAimFriction = 0.0f;
	CurrentAimMagnetism = 0.0f;

	// If Aim assist should only work with game pad? Check if the game pad is in use
	if (bUseOnlyOnGamepad)
	{
		if (IsUsingGamepad() == false)
			return;
	}

	// Get list of valid targets
	const TArray<FAimAssistTarget> ValidTargetList = GetValidTargets();

	if (!ValidTargetList.IsEmpty())
	{
		// find the closest target
		FindBestFrontFacingTarget(ValidTargetList, BestTargetData);

		if (IsValid(BestTargetData.Component))
		{
			UE_LOG(LogTemp, Log, TEXT("VALID BEST TARGET -> %s"), *BestTargetData.Component->GetName());

			FVector2D TargetScreenLoc;
			if (PlayerController->ProjectWorldLocationToScreen(BestTargetData.SocketLocation, TargetScreenLoc, true))
			{
				const float DistanceSq = FVector2D::DistSquared(TargetScreenLoc,
				                                                (GetViewportCenter() + OffsetFromCenter));
				const FVector ToTargetDir = (BestTargetData.SocketLocation - PlayerCameraManager->GetCameraLocation()).
					GetSafeNormal();

				if (bEnableFriction)
					CalculateFriction(BestTargetData, DistanceSq);

				if (bEnableMagnetism)
				{
					CalculateMagnetism(BestTargetData, DistanceSq);
					ApplyMagnetism(DeltaTime, BestTargetData.SocketLocation, ToTargetDir);
				}
			}
		}
	}
}

void UAimAssistComponent::EnableAimAssist(bool bEnabled)
{
	SetComponentTickEnabled(bEnabled);
	bAimAssistEnabled = bEnabled;

	// Reset the values
	CurrentAimFriction = 0.0f;
	CurrentAimMagnetism = 0.0f;
}

bool UAimAssistComponent::IsUsingGamepad() const
{
	return LastInputDevice == EHardwareDevicePrimaryType::Gamepad;
}

void UAimAssistComponent::OnHardwareDeviceChanged(const FPlatformUserId UserId, const FInputDeviceId DeviceId)
{
	auto InputDeviceSubsystem = GEngine->GetEngineSubsystem<UInputDeviceSubsystem>();
	LastInputDevice = InputDeviceSubsystem->GetInputDeviceHardwareIdentifier(DeviceId).PrimaryDeviceType;
}

TArray<FAimAssistTarget> UAimAssistComponent::GetValidTargets()
{
	TArray<FAimAssistTarget> ValidTargets;

	// Get the largest radius from all the aim assist components
	const TArray AimAssistZones{FrictionRadius, MagnetismRadius};
	const float LargestAimAssistZone = FMath::Max(AimAssistZones);

	// Screen center
	FVector2D ScreenCenter = GetViewportCenter();
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

		if (bQueryForTeams)
		{
			if (bGetTeamFromNativeInterface)
			{
				const auto TeamInterface = Cast<IGenericTeamAgentInterface>(Hit.GetActor());
				if (!TeamInterface)
					continue;

				// compare
				const auto TeamId = TeamInterface->GetGenericTeamId();
				if (!TeamsToQuery.Contains(TeamId))
					continue;
			}
			else
			{
				// Check if the hit actor has team identity component
				const auto TeamIdComp = Hit.GetActor()->GetComponentByClass<UTeamIdentityComponent>();
				if (!IsValid(TeamIdComp))
					continue;

				const auto TeamId = TeamIdComp->GetGenericTeamId();
				if (!TeamsToQuery.Contains(TeamId))
					continue;
			}
		}

		// Get all the hit assistance targets on actor
		const TArray<FAimAssistTarget> AimAssistTargets = IAimTargetInterface::Execute_GetAimAssistTargets(
			Hit.GetActor());

		// Loop over the assist targets
		for (const auto& AimAssistTarget : AimAssistTargets)
		{
			if (AimAssistTarget.Sockets.IsEmpty())
				continue;

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
					if (GetWorld()->LineTraceSingleByChannel(OutVisibilityHit, StartLoc, SocketLoc, ECC_Visibility,
					                                         VisibilityQueryParams))
					{
						// Check if the hit component is same as the current aim assist component
						if (OutVisibilityHit.GetComponent() == AimAssistTarget.Component)
							TargetData.Sockets.Add(Socket); // Add socket to list
						else
							DebugTraceColor = FColor::Red;
						if (bShowDebug)
							DrawDebugLine(GetWorld(), StartLoc, OutVisibilityHit.Location, DebugTraceColor, false,
							              0.0f);
					}
				}
			}

			// Add the target data into the aim list
			ValidTargets.Add(TargetData);
		}
	}

	return ValidTargets;
}

bool UAimAssistComponent::IsTargetWithinScreenCircle(const FVector& TargetLoc, const FVector2D& ScreenPoint,
                                                     const float Radius)
{
	FVector2D TargetScreenLoc;
	if (!PlayerController->ProjectWorldLocationToScreen(TargetLoc, TargetScreenLoc, true))
		return false;

	// Distance squared between target and point on screen
	const float DistanceSq = FVector2D::DistSquared(TargetScreenLoc, ScreenPoint);

	// Check if the target point falls in the radius
	return DistanceSq <= FMath::Square(Radius);
}

void UAimAssistComponent::FindBestFrontFacingTarget(const TArray<FAimAssistTarget>& Targets,
                                                    FAimTargetData& OutTargetData)
{
	float BestScore = -FLT_MAX;
	FAimTargetData BestTarget;

	for (const auto& Target : Targets)
	{
		for (const auto& Socket : Target.Sockets)
		{
			const FVector SocketLoc = Target.Component->GetSocketLocation(Socket);
			const FVector ToTarget = SocketLoc - PlayerCameraManager->GetCameraLocation();
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

FVector2D UAimAssistComponent::GetViewportCenter() const
{
	check(PlayerController);

	int SizeX, SizeY;
	PlayerController->GetViewportSize(SizeX, SizeY);
	return FVector2D(SizeX * 0.5f, SizeY * 0.5f);
}

void UAimAssistComponent::CalculateFriction(FAimTargetData Target, float DistanceSqFromOrigin)
{
	const float FrictionRadiusSq = FMath::Square(FrictionRadius);

	if (DistanceSqFromOrigin < FrictionRadiusSq)
	{
		// Calculate scale depending on how close we are to the center
		float FrictionScale = 1.0f - (DistanceSqFromOrigin / FrictionRadiusSq);
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

float UAimAssistComponent::GetCurrentAimFriction() const
{
	return 1.0f - FMath::Min(CurrentAimFriction, 0.9f);
}

void UAimAssistComponent::CalculateMagnetism(FAimTargetData Target, float DistanceSqFromOrigin)
{
	const float MagnetismRadiusSq = FMath::Square(MagnetismRadius);

	if (DistanceSqFromOrigin < MagnetismRadiusSq)
	{
		// Magnetic factor depending on how close to the center of screen
		float MagnetismScale = 1.0f - (DistanceSqFromOrigin / MagnetismRadiusSq);
		// Clamp
		MagnetismScale = FMath::Clamp(MagnetismScale, 0.0f, 1.0f);

		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red,
		                                 FString::Printf(
			                                 TEXT("Magnetism Rate For Active Target : %.2f"), MagnetismScale));

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

void UAimAssistComponent::ApplyMagnetism(const float DeltaTime, const FVector& TargetLocation,
                                         const FVector& TargetDirection) const
{
	if (CurrentAimMagnetism == 0.0f || bEnableMagnetism == false)
		return;

	// Get current control rotation
	const FRotator CurrentRotation = PlayerController->GetControlRotation();
	const FRotator TargetRotation = TargetDirection.Rotation();

	const float RotationSpeed = CurrentAimMagnetism;

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaTime,
		RotationSpeed
	);

	// Apply the new rotation
	PlayerController->SetControlRotation(NewRotation);
}
