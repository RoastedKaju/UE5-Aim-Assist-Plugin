// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AimAssist : ModuleRules
{
    public AimAssist(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new string[] {"Core", "CoreUObject", "InputCore", "Engine", "UMG", "GameplayTags"});
    }
}