// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenAIAssistantComponent.h"
#include "OpenAIClient.h"

UOpenAIAssistantComponent::UOpenAIAssistantComponent()
{
	// This component does not need to tick.
	PrimaryComponentTick.bCanEverTick = false;

	// Set default values
	ApiBaseUrl = TEXT("https://api.openai.com");
	ModelOrAssistantId = TEXT("gpt-4");
	ApiKey = TEXT("");
}

void UOpenAIAssistantComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize the OpenAI client
	OpenAIClient = MakeShareable(new FOpenAIClient());
}

void UOpenAIAssistantComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up the client
	OpenAIClient.Reset();

	Super::EndPlay(EndPlayReason);
}

void UOpenAIAssistantComponent::SendTextToAssistant(const FString& UserMessage)
{
	// Validate configuration
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] ApiKey is not set. Please configure the component."));
		OnAssistantError.Broadcast(TEXT("ApiKey is not configured."));
		return;
	}

	if (ModelOrAssistantId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] ModelOrAssistantId is not set. Please configure the component."));
		OnAssistantError.Broadcast(TEXT("ModelOrAssistantId is not configured."));
		return;
	}

	if (UserMessage.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] UserMessage is empty."));
		OnAssistantError.Broadcast(TEXT("User message cannot be empty."));
		return;
	}

	if (!OpenAIClient.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[OpenAIAssistantComponent] OpenAI client is not initialized."));
		OnAssistantError.Broadcast(TEXT("OpenAI client is not initialized."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[OpenAIAssistantComponent] Sending text to assistant: %s"), *UserMessage);

	// Create a weak reference to this component for the callback
	TWeakObjectPtr<UOpenAIAssistantComponent> WeakThis(this);

	// Send the request
	OpenAIClient->SendTextRequest(
		UserMessage,
		ModelOrAssistantId,
		ApiKey,
		ApiBaseUrl,
		FOnOpenAITextResponse::CreateLambda(
			[WeakThis](bool bSuccess, const FString& ResponseText)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleOpenAIResponse(bSuccess, ResponseText);
				}
			}));
}

void UOpenAIAssistantComponent::SendWavToAssistant(const TArray<uint8>& WavData, const FString& FileName)
{
	// Validate configuration
	if (ApiKey.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] ApiKey is not set. Please configure the component."));
		OnAssistantError.Broadcast(TEXT("ApiKey is not configured."));
		return;
	}

	if (ModelOrAssistantId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] ModelOrAssistantId is not set. Please configure the component."));
		OnAssistantError.Broadcast(TEXT("ModelOrAssistantId is not configured."));
		return;
	}

	if (WavData.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] WavData is empty."));
		OnAssistantError.Broadcast(TEXT("WAV data cannot be empty."));
		return;
	}

	if (!OpenAIClient.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[OpenAIAssistantComponent] OpenAI client is not initialized."));
		OnAssistantError.Broadcast(TEXT("OpenAI client is not initialized."));
		return;
	}

	FString ActualFileName = FileName.IsEmpty() ? TEXT("audio.wav") : FileName;
	UE_LOG(LogTemp, Log, TEXT("[OpenAIAssistantComponent] Sending WAV to assistant: %s (%d bytes)"), *ActualFileName, WavData.Num());

	// Create a weak reference to this component for the callback
	TWeakObjectPtr<UOpenAIAssistantComponent> WeakThis(this);

	// Send the request
	OpenAIClient->SendAudioRequest(
		WavData,
		ActualFileName,
		ModelOrAssistantId,
		ApiKey,
		ApiBaseUrl,
		FOnOpenAITextResponse::CreateLambda(
			[WeakThis](bool bSuccess, const FString& ResponseText)
			{
				if (WeakThis.IsValid())
				{
					WeakThis->HandleOpenAIResponse(bSuccess, ResponseText);
				}
			}));
}

void UOpenAIAssistantComponent::HandleOpenAIResponse(bool bSuccess, const FString& ResponseText)
{
	// This function is called on the game thread by the OpenAI client
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[OpenAIAssistantComponent] Received response: %s"), *ResponseText);
		OnAssistantTextResponse.Broadcast(ResponseText);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenAIAssistantComponent] Request failed: %s"), *ResponseText);
		OnAssistantError.Broadcast(ResponseText);
	}
}
