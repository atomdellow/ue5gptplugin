// Copyright Epic Games, Inc. All Rights Reserved.

#include "Modules/ModuleManager.h"

/**
 * Module implementation for OpenAIAssistantBridge plugin.
 * This module provides integration with OpenAI's Assistants/Responses API.
 */
class FOpenAIAssistantBridgeModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		// Plugin startup logic goes here.
		// Currently empty - extend as needed for initialization.
	}

	virtual void ShutdownModule() override
	{
		// Plugin shutdown logic goes here.
		// Currently empty - extend as needed for cleanup.
	}
};

IMPLEMENT_MODULE(FOpenAIAssistantBridgeModule, OpenAIAssistantBridge)
