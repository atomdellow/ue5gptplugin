// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OpenAIAssistantBridge : ModuleRules
{
	public OpenAIAssistantBridge(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"HTTP",
			"Json",
			"JsonUtilities"
		});
	}
}
