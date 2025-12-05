// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenAIClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/Guid.h"
#include "Async/Async.h"

// ============================================================================
// API ENDPOINT CONFIGURATION
// TODO: Verify these endpoints with the latest OpenAI docs.
// Adjust these paths as needed when the API changes.
// ============================================================================

// Text/Chat completions endpoint - uses the Chat Completions API
// For the Assistants API, you would need different endpoints: /v1/assistants, /v1/threads, /v1/runs
static const FString TextEndpointPath = TEXT("/v1/chat/completions");

// Audio transcription endpoint (Whisper API)
static const FString AudioTranscriptionEndpointPath = TEXT("/v1/audio/transcriptions");

// Default transcription model
static const FString DefaultWhisperModel = TEXT("whisper-1");

// ============================================================================

FOpenAIClient::FOpenAIClient()
{
}

FOpenAIClient::~FOpenAIClient()
{
}

void FOpenAIClient::SendTextRequest(
	const FString& UserMessage,
	const FString& ModelOrAssistantId,
	const FString& ApiKey,
	const FString& ApiBaseUrl,
	const FOnOpenAITextResponse& CompletionCallback)
{
	// Build the full URL
	// Note: To use the Assistants API instead of Chat Completions, you would need to:
	// 1. Create an assistant via /v1/assistants
	// 2. Create a thread via /v1/threads
	// 3. Add messages and run via /v1/threads/{id}/messages and /v1/threads/{id}/runs
	FString Url = ApiBaseUrl + TextEndpointPath;

	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));

	// Build JSON request body following the Chat Completions API format
	// See: https://platform.openai.com/docs/api-reference/chat/create
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	RootObject->SetStringField(TEXT("model"), ModelOrAssistantId);

	// Build messages array
	TArray<TSharedPtr<FJsonValue>> MessagesArray;

	TSharedPtr<FJsonObject> UserMessageObject = MakeShareable(new FJsonObject);
	UserMessageObject->SetStringField(TEXT("role"), TEXT("user"));
	UserMessageObject->SetStringField(TEXT("content"), UserMessage);
	MessagesArray.Add(MakeShareable(new FJsonValueObject(UserMessageObject)));

	RootObject->SetArrayField(TEXT("messages"), MessagesArray);

	// Serialize to JSON string
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);

	UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Sending text request to: %s"), *Url);
	UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Model: %s"), *ModelOrAssistantId);

	// Bind completion handler
	Request->OnProcessRequestComplete().BindLambda(
		[CompletionCallback](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bWasSuccessful)
		{
			FString ResponseText;
			bool bSuccess = false;

			if (!bWasSuccessful || !Res.IsValid())
			{
				ResponseText = TEXT("HTTP request failed or response is invalid.");
				UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
			}
			else
			{
				int32 StatusCode = Res->GetResponseCode();
				FString ResponseContent = Res->GetContentAsString();

				UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] HTTP Status: %d"), StatusCode);

				if (StatusCode >= 200 && StatusCode < 300)
				{
					// Parse JSON response
					TSharedPtr<FJsonObject> JsonObject;
					TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

					if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
					{
						// Parse Chat Completions API response format:
						// { "choices": [ { "message": { "content": "..." } } ] }
						// See: https://platform.openai.com/docs/api-reference/chat/object
						const TArray<TSharedPtr<FJsonValue>>* ChoicesArray;
						if (JsonObject->TryGetArrayField(TEXT("choices"), ChoicesArray) && ChoicesArray->Num() > 0)
						{
							TSharedPtr<FJsonObject> FirstChoice = (*ChoicesArray)[0]->AsObject();
							if (FirstChoice.IsValid())
							{
								TSharedPtr<FJsonObject> MessageObject = FirstChoice->GetObjectField(TEXT("message"));
								if (MessageObject.IsValid())
								{
									ResponseText = MessageObject->GetStringField(TEXT("content"));
									bSuccess = true;
									UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Successfully parsed response."));
								}
							}
						}

						if (!bSuccess)
						{
							// Check for error in response
							TSharedPtr<FJsonObject> ErrorObject = JsonObject->GetObjectField(TEXT("error"));
							if (ErrorObject.IsValid())
							{
								ResponseText = ErrorObject->GetStringField(TEXT("message"));
							}
							else
							{
								ResponseText = TEXT("Failed to parse response: unexpected JSON structure.");
							}
							UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
						}
					}
					else
					{
						ResponseText = TEXT("Failed to parse JSON response.");
						UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
					}
				}
				else
				{
					// Non-2xx status code
					ResponseText = FString::Printf(TEXT("API error (HTTP %d): %s"), StatusCode, *ResponseContent);
					UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
				}
			}

			// Execute callback on game thread
			AsyncTask(ENamedThreads::GameThread, [CompletionCallback, bSuccess, ResponseText]()
			{
				CompletionCallback.ExecuteIfBound(bSuccess, ResponseText);
			});
		});

	// Send the request
	Request->ProcessRequest();
}

void FOpenAIClient::SendAudioRequest(
	const TArray<uint8>& WavData,
	const FString& FileName,
	const FString& ModelOrAssistantId,
	const FString& ApiKey,
	const FString& ApiBaseUrl,
	const FOnOpenAITextResponse& CompletionCallback)
{
	// First, transcribe the audio using Whisper API
	TranscribeAudio(
		WavData,
		FileName,
		ApiKey,
		ApiBaseUrl,
		FOnOpenAITextResponse::CreateLambda(
			[this, ModelOrAssistantId, ApiKey, ApiBaseUrl, CompletionCallback]
			(bool bTranscriptionSuccess, const FString& TranscribedText)
			{
				if (!bTranscriptionSuccess)
				{
					// Transcription failed, propagate error
					AsyncTask(ENamedThreads::GameThread, [CompletionCallback, TranscribedText]()
					{
						CompletionCallback.ExecuteIfBound(false, TranscribedText);
					});
					return;
				}

				UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Audio transcribed: %s"), *TranscribedText);

				// Now send the transcribed text to the text API
				SendTextRequest(
					TranscribedText,
					ModelOrAssistantId,
					ApiKey,
					ApiBaseUrl,
					CompletionCallback);
			}));
}

void FOpenAIClient::TranscribeAudio(
	const TArray<uint8>& WavData,
	const FString& FileName,
	const FString& ApiKey,
	const FString& ApiBaseUrl,
	const FOnOpenAITextResponse& TranscriptionCallback)
{
	// Build the full URL
	FString Url = ApiBaseUrl + AudioTranscriptionEndpointPath;

	// Generate boundary for multipart form data
	FString Boundary = GenerateBoundary();

	// Create HTTP request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *ApiKey));
	Request->SetHeader(TEXT("Content-Type"), FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary));

	// Build multipart form data
	TArray<uint8> FormData = BuildMultipartFormData(WavData, FileName, Boundary);
	Request->SetContent(FormData);

	UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Sending audio transcription request to: %s"), *Url);
	UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Audio file: %s, Size: %d bytes"), *FileName, WavData.Num());

	// Bind completion handler
	Request->OnProcessRequestComplete().BindLambda(
		[TranscriptionCallback](FHttpRequestPtr Req, FHttpResponsePtr Res, bool bWasSuccessful)
		{
			FString ResponseText;
			bool bSuccess = false;

			if (!bWasSuccessful || !Res.IsValid())
			{
				ResponseText = TEXT("Audio transcription HTTP request failed or response is invalid.");
				UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
			}
			else
			{
				int32 StatusCode = Res->GetResponseCode();
				FString ResponseContent = Res->GetContentAsString();

				UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Transcription HTTP Status: %d"), StatusCode);

				if (StatusCode >= 200 && StatusCode < 300)
				{
					// Parse JSON response
					TSharedPtr<FJsonObject> JsonObject;
					TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseContent);

					if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
					{
						// Parse Whisper API transcription response: { "text": "..." }
						// See: https://platform.openai.com/docs/api-reference/audio/createTranscription
						if (JsonObject->TryGetStringField(TEXT("text"), ResponseText))
						{
							bSuccess = true;
							UE_LOG(LogTemp, Log, TEXT("[OpenAIClient] Successfully transcribed audio."));
						}
						else
						{
							// Check for error
							TSharedPtr<FJsonObject> ErrorObject = JsonObject->GetObjectField(TEXT("error"));
							if (ErrorObject.IsValid())
							{
								ResponseText = ErrorObject->GetStringField(TEXT("message"));
							}
							else
							{
								ResponseText = TEXT("Failed to parse transcription response: 'text' field not found.");
							}
							UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
						}
					}
					else
					{
						ResponseText = TEXT("Failed to parse transcription JSON response.");
						UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
					}
				}
				else
				{
					ResponseText = FString::Printf(TEXT("Transcription API error (HTTP %d): %s"), StatusCode, *ResponseContent);
					UE_LOG(LogTemp, Warning, TEXT("[OpenAIClient] %s"), *ResponseText);
				}
			}

			// Execute callback on game thread
			AsyncTask(ENamedThreads::GameThread, [TranscriptionCallback, bSuccess, ResponseText]()
			{
				TranscriptionCallback.ExecuteIfBound(bSuccess, ResponseText);
			});
		});

	// Send the request
	Request->ProcessRequest();
}

FString FOpenAIClient::GenerateBoundary() const
{
	return FString::Printf(TEXT("----UnrealBoundary%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits));
}

TArray<uint8> FOpenAIClient::BuildMultipartFormData(
	const TArray<uint8>& WavData,
	const FString& FileName,
	const FString& Boundary) const
{
	TArray<uint8> FormData;
	FString FormDataString;

	// Add 'file' field (the audio data)
	FormDataString += FString::Printf(TEXT("--%s\r\n"), *Boundary);
	FormDataString += FString::Printf(TEXT("Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n"), *FileName);
	FormDataString += TEXT("Content-Type: audio/wav\r\n\r\n");

	// Convert string part to bytes
	FTCHARToUTF8 Converter(*FormDataString);
	FormData.Append((const uint8*)Converter.Get(), Converter.Length());

	// Append the raw WAV data
	FormData.Append(WavData);

	// Add 'model' field
	FString ModelFieldString;
	ModelFieldString += TEXT("\r\n");
	ModelFieldString += FString::Printf(TEXT("--%s\r\n"), *Boundary);
	ModelFieldString += TEXT("Content-Disposition: form-data; name=\"model\"\r\n\r\n");
	ModelFieldString += DefaultWhisperModel;

	FTCHARToUTF8 ModelConverter(*ModelFieldString);
	FormData.Append((const uint8*)ModelConverter.Get(), ModelConverter.Length());

	// Add closing boundary
	FString ClosingBoundary = FString::Printf(TEXT("\r\n--%s--\r\n"), *Boundary);
	FTCHARToUTF8 ClosingConverter(*ClosingBoundary);
	FormData.Append((const uint8*)ClosingConverter.Get(), ClosingConverter.Length());

	return FormData;
}
