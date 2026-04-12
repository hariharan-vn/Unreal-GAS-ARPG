// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GAS_ARPG : ModuleRules
{
	public GAS_ARPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "NavigationSystem", "AIModule", "Niagara", "EnhancedInput","GameplayAbilities", "GameplayTasks", "GameplayTags" });
	}
}