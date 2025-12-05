// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "OpenAIAssistantComponent.generated.h"

// Forward declaration
class FOpenAIClient;

/**
 * Delegate for receiving text responses from OpenAI.
 * Broadcast when a successful response is received.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpenAITextResponseBP, const FString&, ResponseText);

/**
 * Delegate for receiving error messages.
 * Broadcast when a request fails.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOpenAIErrorBP, const FString&, ErrorMessage);

/**
 * UOpenAIAssistantComponent - Blueprint-facing component for OpenAI integration.
 * 
 * This component can be attached to any actor to enable communication with OpenAI's
 * Assistants/Responses API. It supports both text and audio (WAV) input.
 * 
 * Usage:
 * 1. Add this component to an actor.
 * 2. Set ApiKey, ModelOrAssistantId, and optionally ApiBaseUrl.
 * 3. Bind to OnAssistantTextResponse and OnAssistantError events.
 * 4. Call SendTextToAssistant() or SendWavToAssistant() from Blueprints.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Category="OpenAI")
class OPENAIASSISTANTBRIDGE_API UOpenAIAssistantComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UOpenAIAssistantComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// ========================================================================
	// CONFIGURATION PROPERTIES
	// ========================================================================

	/**
	 * Your OpenAI API key.
	 * Keep this secure - consider loading from a config file or environment variable.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI")
	FString ApiKey;

	/**
	 * The model or assistant ID to use for requests.
	 * Examples: "gpt-4", "gpt-3.5-turbo", or an assistant ID for the Assistants API.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI")
	FString ModelOrAssistantId;

	/**
	 * Base URL for the OpenAI API.
	 * Default: "https://api.openai.com"
	 * Change this if using a proxy or alternative API-compatible endpoint.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="OpenAI")
	FString ApiBaseUrl;

	// ========================================================================
	// BLUEPRINT-ASSIGNABLE DELEGATES (EVENTS)
	// ========================================================================

	/**
	 * Event broadcast when a text response is successfully received from OpenAI.
	 * Bind to this in Blueprints to handle responses.
	 */
	UPROPERTY(BlueprintAssignable, Category="OpenAI")
	FOnOpenAITextResponseBP OnAssistantTextResponse;

	/**
	 * Event broadcast when an error occurs during an OpenAI request.
	 * Bind to this in Blueprints to handle errors.
	 */
	UPROPERTY(BlueprintAssignable, Category="OpenAI")
	FOnOpenAIErrorBP OnAssistantError;

	// ========================================================================
	// BLUEPRINT-CALLABLE FUNCTIONS
	// ========================================================================

	/**
	 * Send a text message to OpenAI and receive a response.
	 * The response will be broadcast via OnAssistantTextResponse.
	 * Errors will be broadcast via OnAssistantError.
	 * 
	 * @param UserMessage - The text message to send to the assistant.
	 */
	UFUNCTION(BlueprintCallable, Category="OpenAI")
	void SendTextToAssistant(const FString& UserMessage);

	/**
	 * Send WAV audio data to OpenAI for transcription and response.
	 * The audio is first transcribed using Whisper, then the transcribed text
	 * is sent to the text API. The response will be broadcast via OnAssistantTextResponse.
	 * 
	 * @param WavData - Raw WAV audio data as a byte array.
	 * @param FileName - A filename for the audio (e.g., "recording.wav").
	 */
	UFUNCTION(BlueprintCallable, Category="OpenAI")
	void SendWavToAssistant(const TArray<uint8>& WavData, const FString& FileName);

private:
	/** Internal HTTP client for making OpenAI API calls */
	TSharedPtr<FOpenAIClient> OpenAIClient;

	/**
	 * Internal callback handler for API responses.
	 * Routes success/failure to the appropriate delegate.
	 */
	void HandleOpenAIResponse(bool bSuccess, const FString& ResponseText);
};
