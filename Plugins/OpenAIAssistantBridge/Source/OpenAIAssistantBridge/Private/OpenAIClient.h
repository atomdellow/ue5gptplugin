// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

/**
 * Delegate for OpenAI text response callbacks.
 * @param bSuccess - Whether the request was successful.
 * @param ResponseText - The response text from OpenAI, or an error message on failure.
 */
DECLARE_DELEGATE_TwoParams(FOnOpenAITextResponse, bool /*bSuccess*/, const FString& /*ResponseText*/);

/**
 * FOpenAIClient - HTTP wrapper for OpenAI API calls.
 * 
 * This class encapsulates all HTTP communication with OpenAI's API endpoints.
 * It provides async methods for text and audio requests.
 */
class FOpenAIClient
{
public:
	FOpenAIClient();
	~FOpenAIClient();

	/**
	 * Send a text request to OpenAI's API.
	 * 
	 * @param UserMessage - The user's message/prompt to send.
	 * @param ModelOrAssistantId - The model or assistant ID to use (e.g., "gpt-4", "gpt-3.5-turbo").
	 * @param ApiKey - Your OpenAI API key.
	 * @param ApiBaseUrl - Base URL for the API (default: "https://api.openai.com").
	 * @param CompletionCallback - Callback invoked when the request completes.
	 */
	void SendTextRequest(
		const FString& UserMessage,
		const FString& ModelOrAssistantId,
		const FString& ApiKey,
		const FString& ApiBaseUrl,
		const FOnOpenAITextResponse& CompletionCallback);

	/**
	 * Send a WAV audio file to OpenAI for transcription, then send the transcribed text to the API.
	 * 
	 * @param WavData - Raw WAV audio data as bytes.
	 * @param FileName - Original filename for the audio (e.g., "recording.wav").
	 * @param ModelOrAssistantId - The model or assistant ID to use for the text response.
	 * @param ApiKey - Your OpenAI API key.
	 * @param ApiBaseUrl - Base URL for the API (default: "https://api.openai.com").
	 * @param CompletionCallback - Callback invoked when the full request completes.
	 */
	void SendAudioRequest(
		const TArray<uint8>& WavData,
		const FString& FileName,
		const FString& ModelOrAssistantId,
		const FString& ApiKey,
		const FString& ApiBaseUrl,
		const FOnOpenAITextResponse& CompletionCallback);

private:
	/**
	 * Transcribe audio using OpenAI's Whisper API.
	 * 
	 * @param WavData - Raw WAV audio data.
	 * @param FileName - Original filename.
	 * @param ApiKey - OpenAI API key.
	 * @param ApiBaseUrl - Base URL for the API.
	 * @param TranscriptionCallback - Callback with transcribed text.
	 */
	void TranscribeAudio(
		const TArray<uint8>& WavData,
		const FString& FileName,
		const FString& ApiKey,
		const FString& ApiBaseUrl,
		const FOnOpenAITextResponse& TranscriptionCallback);

	/**
	 * Build the multipart form data boundary string.
	 */
	FString GenerateBoundary() const;

	/**
	 * Build multipart form data for audio upload.
	 */
	TArray<uint8> BuildMultipartFormData(
		const TArray<uint8>& WavData,
		const FString& FileName,
		const FString& Boundary) const;
};
